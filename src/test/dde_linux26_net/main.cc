/*
 * \brief   DDE Linux 2.6 NET test program
 * \author  Christian Helmuth
 * \date    2009-02-09
 *
 * I would like to send big thanks to Björn Döbel who implemented the ORe
 * ethernet driver server of the TUD:OS group published under the terms of GNU
 * GPL.
 */

#include <base/env.h>
#include <base/exception.h>
#include <base/printf.h>
#include <base/sleep.h>

#include <util/fifo.h>
#include <util/string.h>

extern "C" {
#include <dde_kit/lock.h>
#include <dde_kit/printf.h>
#include <dde_kit/timer.h>

#include <dde_linux26/general.h>
#include <dde_linux26/net.h>
}

#include "local.h"

using namespace Genode;


/*
 * Automatically generated by "os/tool/dde_kit_find_initcalls bin/test-dde_linux26_net".
 */
extern int (*dde_kit_initcall_1_dde_linux26_page_cache_init)(void);
extern int (*dde_kit_initcall_1_helper_init)(void);
extern int (*dde_kit_initcall_2_pci_driver_init)(void);
extern int (*dde_kit_initcall_2_pcibus_class_init)(void);
extern int (*dde_kit_initcall_4__call_init_workqueues)(void);
extern int (*dde_kit_initcall_4_dde_linux26_init_pci)(void);
extern int (*dde_kit_initcall_4_net_dev_init)(void);
extern int (*dde_kit_initcall_6_pci_init)(void);
extern int (*dde_kit_initcall_6_pcnet32_init_module)(void);
extern int (*dde_kit_initcall_6_ne2k_pci_init)(void);

void do_initcalls(void)
{
	dde_kit_initcall_1_dde_linux26_page_cache_init();
	dde_kit_initcall_1_helper_init();
	dde_kit_initcall_2_pci_driver_init();
	dde_kit_initcall_2_pcibus_class_init();;
	dde_kit_initcall_4__call_init_workqueues();
	dde_kit_initcall_4_dde_linux26_init_pci();
	dde_kit_initcall_4_net_dev_init();
	dde_kit_initcall_6_pci_init();
	dde_kit_initcall_6_pcnet32_init_module();
	dde_kit_initcall_6_ne2k_pci_init();
}
/*
 * End of automatically generated code.
 */


/*******************************
 ** Packet reception handling **
 *******************************/

class Rx_packet : public Fifo<Rx_packet>::Element
{
	private:

		enum { BUFFER_SIZE = 1514 };

		char     _buffer[BUFFER_SIZE];
		unsigned _len;

	public:

		Rx_packet(const unsigned char *packet, unsigned len)
		: _len(len)
		{
			if (len > BUFFER_SIZE) throw Exception();
			memcpy(_buffer, packet, len);
		}

		char * buffer() { return _buffer; }
		unsigned len()  { return _len; }
};


static Fifo<Rx_packet> * rx_packet_pool()
{
	static Fifo<Rx_packet> _pool;

	return &_pool;
}


static void rx_handler(unsigned if_index, const unsigned char *packet, unsigned packet_len)
{
	try {
		Rx_packet *p = new (env()->heap()) Rx_packet(packet, packet_len);
		rx_packet_pool()->enqueue(p);
	} catch (...) {
		PERR("Out of memory");
	}
}


/********************************
 ** Driver API for uIP library **
 ********************************/

void dev_send(const unsigned char *packet, unsigned packet_len)
{
	dde_linux26_net_tx(1, packet, packet_len);
}


unsigned dev_recv(unsigned char *buffer, unsigned buffer_len)
{
	/* XXX this could be blocking */

	Rx_packet *packet = rx_packet_pool()->dequeue();
	if (!packet) return 0;

	/* XXX drop oversized packets */
	if (packet->len() > buffer_len) {
		PERR("dropping packet of size %d (max uIP size is %d)", packet->len(), buffer_len);
		destroy(env()->heap(), packet);
		return 0;
	}

	unsigned len = packet->len();

	memcpy(buffer, packet->buffer(), len);
	destroy(env()->heap(), packet);

	return len;
}


/******************
 ** Main program **
 ******************/

int main(int argc, char **argv)
{
	dde_linux26_init();

	PDBG("--- initcalls");
	do_initcalls();

	PDBG("--- init NET");
	int cnt = dde_linux26_net_init();
	PDBG("  number of devices: %d", cnt);

	PDBG("--- init rx_callbacks");
	dde_linux26_net_register_rx_callback(rx_handler);

	static unsigned char mac_addr[6];
	dde_linux26_net_get_mac_addr(1, mac_addr);

	uip_loop(mac_addr);

	sleep_forever();
	return 0;
}
