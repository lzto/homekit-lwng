#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>

#include <libmdnsd/mdnsd.h>
#include <libmdnsd/sdtxt.h>


#ifndef MDNS_TTL
#define MDNS_TTL 4500
#endif


uint32_t homekit_random() {
    static bool initialized = false;
    printf("called homekit_random\n");
    if (initialized)
    {
        initialized = true;
        srand(time(0));
    }
    return random();
}

void homekit_random_fill(uint8_t *data, size_t size) {
    printf("called homekit_random_fill\n");
    uint32_t x;
    for (int i=0; i<size; i+=sizeof(x)) {
        x = homekit_random();
        memcpy(data+i, &x, (size-i >= sizeof(x)) ? sizeof(x) : size-i);
    }
}

void homekit_system_restart() {
}

///////////////////////////////////////////////////////////////
volatile sig_atomic_t running = 1;
static mdns_daemon_t *mdnsd = NULL;
static pthread_t mdns_tid;
static pthread_mutex_t mutex_reload;
static char hlocal[256], nlocal[256], tlocal[256];

extern int getaddr(char *iface, struct in_addr *ina);

static int iface_init(char *iface, struct in_addr *ina)
{
	memset(ina, 0, sizeof(*ina));
	return getaddr(iface, ina);
}

void mdnsd_conflict(char *name, int type, void *arg)
{
	ERR("conflicting name detected %s for type %d, dropping record ...", name, type);
}


/* Create multicast 224.0.0.251:5353 socket */
static int multicast_socket(struct in_addr ina, unsigned char ttl)
{
	struct sockaddr_in sin;
	struct ip_mreq mc;
	socklen_t len;
	int unicast_ttl = 255;
	int sd, bufsiz, flag = 1;

	sd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (sd < 0)
		return -1;

#ifdef SO_REUSEPORT
	setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
#endif
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	/* Double the size of the receive buffer (getsockopt() returns the double) */
	len = sizeof(bufsiz);
	if (!getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &bufsiz, &len))
		setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &bufsiz, sizeof(bufsiz));

	/* Set interface for outbound multicast */
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, &ina, sizeof(ina)))
		WARN("Failed setting IP_MULTICAST_IF to %s: %s",
		     inet_ntoa(ina), strerror(errno));

	/*
	 * All traffic on 224.0.0.* is link-local only, so the default
	 * TTL is set to 1.  Some users may however want to route mDNS.
	 */
	setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	/* mDNS also supports unicast, so we need a relevant TTL there too */
	setsockopt(sd, IPPROTO_IP, IP_TTL, &unicast_ttl, sizeof(unicast_ttl));

	/* Filter inbound traffic from anyone (ANY) to port 5353 */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(5353);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&sin, sizeof(sin))) {
		close(sd);
		return -1;
	}

	/*
	 * Join mDNS link-local group on the given interface, that way
	 * we can receive multicast without a proper net route (default
	 * route or a 224.0.0.0/24 net route).
	 */
	mc.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
	mc.imr_interface = ina;
	setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc, sizeof(mc));

	return sd;
}

/* Create a new record, or update an existing one */
mdns_record_t *record(mdns_daemon_t *d, int shared, char *host,
		      const char *name, unsigned short type, unsigned long ttl)
{
	mdns_record_t *r;

	r = mdnsd_find(d, name, type);
	while (r != NULL) {
		const mdns_answer_t *a;

		if (!host)
			break;

		a = mdnsd_record_data(r);
		if (!a) {
			r = mdnsd_record_next(r);
			continue;
		}

		if (a->rdname && strcmp(a->rdname, host)) {
			r = mdnsd_record_next(r);
			continue;
		}

		return r;
	}

	if (!r) {
		if (shared)
			r = mdnsd_shared(d, name, type, ttl);
		else
			r = mdnsd_unique(d, name, type, ttl, mdnsd_conflict, NULL);

		if (host)
			mdnsd_set_host(d, r, host);
	}

	return r;
}

static void record_received(const struct resource *r, void *data)
{
	char ipinput[INET_ADDRSTRLEN];

	switch(r->type) {
	case QTYPE_A:
		inet_ntop(AF_INET, &(r->known.a.ip), ipinput, INET_ADDRSTRLEN);
		DBG("Got %s: A %s->%s", r->name, r->known.a.name, ipinput);
		break;

	case QTYPE_NS:
		DBG("Got %s: NS %s", r->name, r->known.ns.name);
		break;

	case QTYPE_CNAME:
		DBG("Got %s: CNAME %s", r->name, r->known.cname.name);
		break;

	case QTYPE_PTR:
		DBG("Got %s: PTR %s", r->name, r->known.ptr.name);
		break;

	case QTYPE_TXT:
		DBG("Got %s: TXT %s", r->name, r->rdata);
		break;

	case QTYPE_SRV:
		DBG("Got %s: SRV %d %d %d %s", r->name, r->known.srv.priority,
		    r->known.srv.weight, r->known.srv.port, r->known.srv.name);
		break;

	default:
		DBG("Got %s: unknown", r->name);

	}
}



void* homekit_mdns_worker()
{
	int ttl = 255;
	int c, sd, rc;
    struct in_addr ina = { 0 };
    fd_set fds;
    struct timeval tv = { 0 };
    char *iface = NULL;

retry:
	while (iface_init(iface, &ina)) {
        if (iface)
            INFO("No address for interface %s yet ...", iface);
        sleep(1);
	}
	mdnsd_set_address(mdnsd, ina);
	mdnsd_register_receive_callback(mdnsd, record_received, NULL);

	sd = multicast_socket(ina, (unsigned char)ttl);
	if (sd < 0) {
		ERR("Failed creating socket: %s", strerror(errno));
		return NULL;
	}

	while (running) {
		FD_ZERO(&fds);
		FD_SET(sd, &fds);
		rc = select(sd + 1, &fds, NULL, NULL, &tv);
		if (rc < 0 && EINTR == errno) {
			if (!running)
				break;
            //should reload???
            
		}

        pthread_mutex_lock(&mutex_reload);
		/* Check if IP address changed, needed to update A records */
		if (iface) {
			if (iface_init(iface, &ina)) {
				INFO("Interface %s lost its IP address, waiting ...", iface);
				break;
			}
			mdnsd_set_address(mdnsd, ina);
		}

		rc = mdnsd_step(mdnsd, sd, FD_ISSET(sd, &fds), true, &tv);
		if (rc == 1) {
			ERR("Failed reading from socket %d: %s", errno, strerror(errno));
			break;
		}else if (rc == 2) {
			ERR("Failed writing to socket: %s", strerror(errno));
			break;
		}

		DBG("Going back to sleep, for %d sec ...", tv.tv_sec);
        pthread_mutex_unlock(&mutex_reload);
	}

	close(sd);
	if (running) {
		DBG("Restarting ...");
		sleep(1);
		goto retry;
	}
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
	mdns_record_t *r;

    printf("mDNS setup: Name=%s Port=%d\n",
       instance_name, port);

    pthread_mutex_lock(&mutex_reload);

 	snprintf(hlocal, sizeof(hlocal), "%s._hap.local.", instance_name);
	snprintf(nlocal, sizeof(nlocal), "%s.local.", instance_name);
	snprintf(tlocal, sizeof(tlocal), "_hap.local.");

	/* Announce that we have a $type service */
	r = record(mdnsd, 1, tlocal, DISCO_NAME, QTYPE_PTR, 120);
	r = record(mdnsd, 1, hlocal, tlocal, QTYPE_PTR, 120);

	r = record(mdnsd, 0, NULL, hlocal, QTYPE_SRV, 120);
	mdnsd_set_srv(mdnsd, r, 0, 0, port, nlocal);

	r = record(mdnsd, 0, NULL, nlocal, QTYPE_A, 120);
	mdnsd_set_ip(mdnsd, r, mdnsd_get_address(mdnsd));
   
    pthread_mutex_unlock(&mutex_reload);
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);
    
    if (value_len && value_len < sizeof(value)-1) {
        pthread_mutex_lock(&mutex_reload);
	    mdns_record_t *r = record(mdnsd, 0, NULL, hlocal, QTYPE_TXT, 4500);
        xht_t * h = xht_new(11);
        /*for (int i = 0; i < srec.txt_num; i++) {
            char *ptr;

            ptr = strchr(srec.txt[i], '=');
            if (!ptr)
                continue;
            *ptr++ = 0;

            xht_set(h, srec.txt[i], ptr);
        }*/
        printf("TODO! add mdns txt\n");
	    int len = 0;
	    unsigned char *packet = sd2txt(h, &len);
        xht_free(h);
        mdnsd_set_raw(mdnsd, r, (char *)packet, len);
        free(packet);

        //mdns_service_txt_item_set("_hap", "_tcp", key, value);
        pthread_mutex_unlock(&mutex_reload);
    }
    printf("mDNS add txt: key=%s value=%s\n",
       key, value);
}

void homekit_mdns_configure_finalize() {
    printf("mDNS done setup\n");
}

void homekit_mdns_init() {
	mdnsd = mdnsd_new(QCLASS_IN, 1000);
    pthread_mutex_init(&mutex_reload, NULL);

    pthread_create(&mdns_tid, NULL, homekit_mdns_worker, NULL);
}

void homekit_mdns_teardown() {
	mdnsd_shutdown(mdnsd);
	mdnsd_free(mdnsd);
    pthread_exit(NULL);
}


