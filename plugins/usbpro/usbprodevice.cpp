/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * usbprodevice.cpp
 * UsbPro device
 * Copyright (C) 2006  Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one at a time.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <llad/logger.h>
#include <llad/preferences.h>
#include <llad/universe.h>

#include "usbprodevice.h"
#include "usbproport.h"

#include "UsbProConfMsg.h"

#include "UsbProConfMsgPrmReq.h"
#include "UsbProConfMsgPrmRep.h"
#include "UsbProConfMsgSprmReq.h"
#include "UsbProConfMsgSprmRep.h"
#include "UsbProConfMsgSerReq.h"
#include "UsbProConfMsgSerRep.h"


#if HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Create a new device
 *
 * @param owner	the plugin that owns this device
 * @param name	the device name
 * @param dev_path	path to the pro widget
 */
UsbProDevice::UsbProDevice(Plugin *owner, const string &name, const string &dev_path) :
	Device(owner, name),
	m_path(dev_path),
	m_parser(NULL),
	m_widget(NULL) {
		m_parser = new UsbProConfParser();
		m_widget = new UsbProWidget();
}


/*
 * Destroy this device
 */
UsbProDevice::~UsbProDevice() {
	if (m_enabled)
		stop();
		
	if (m_parser != NULL)
		delete m_parser;

	if (m_widget != NULL)
		delete m_widget;
}


/*
 * Start this device
 */
int UsbProDevice::start() {
	UsbProPort *port = NULL;
	Port *prt = NULL;
	int debug = 0;
	int ret;
	
	/* set up ports */
	for(int i=0; i < 2; i++) {
		port = new UsbProPort(this,i);

		if(port != NULL) 
			this->add_port(port);
	}
	
	// connect to the widget
	ret = m_widget->connect(m_path);

	if(ret) {
		Logger::instance()->log(Logger::WARN, "UsbProPlugin: failed to connect to %s", m_path.c_str()  );
		goto e_dev;
	}

	m_widget->set_listener(this);

	m_enabled = true;
	return 0;

e_disconnect:
	m_widget->disconnect();

e_dev:
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i);
		if(prt != NULL) 
			delete prt;
	}
	return -1;
}


/*
 * Stop this device
 */
int UsbProDevice::stop() {
	Port *prt = NULL;
	Universe *uni;
	
	if (!m_enabled)
		return 0;

	// disconnect from widget
	m_widget->disconnect();
	
	for(int i=0; i < port_count() ; i++) {
		prt = get_port(i);
		if(prt != NULL) {
			uni = prt->get_universe();

			if(uni) 
				uni->remove_port(prt);
				
			delete prt;
		}
	}

	m_enabled = false;
	return 0;
}


/*
 * return the sd for this device
 */
int UsbProDevice::get_sd() const {
	return m_widget->fd();
}


/*
 * Called when there is activity on our descriptors
 */
int UsbProDevice::fd_action() {
	m_widget->recv();
}


/*
 * Send the dmx out the widget
 * called from the UsbProPort
 *
 * @return 	0 on success, non 0 on failure
 */
int UsbProDevice::send_dmx(uint8_t *data, int len) {
	return m_widget->send_dmx(data,len);
}


/*
 * Copy the dmx buffer into the arguments
 * Called from the UsbProPort
 *
 * @return 	the length of the dmx data copied
 */
int UsbProDevice::get_dmx(uint8_t *data, int len) const {
	return m_widget->get_dmx(data,len);
}


// call this when something changes
// where to store data to ?
// I'm thinking a config file in /etc/llad/llad.conf
int UsbProDevice::save_config() const {


	return 0;
}


/*
 * For now we can't block in a configure call, so we keep
 * a cache of the widget parameters and return those.
 *
 * @param req		the request data
 * @param reql		the request length
 * @param rep		buffer for the reply
 * @param repl		the length of the reply buffer
 *
 * @return	the length of the reply
 */
uint8_t *UsbProDevice::configure(const uint8_t *request, int reql, int *repl) {

	UsbProConfMsg *m = m_parser->parse(request, reql);
	UsbProConfMsg *r = NULL ;
	uint8_t *reply = NULL;
	*repl = 0;

	printf("type is %i\n" , m->type() ) ;
	switch(m->type()) {
		case LLA_USBPRO_CONF_MSG_PRM_REQ:
			r = config_get_params(dynamic_cast<UsbProConfMsgPrmReq*>(m));
		case LLA_USBPRO_CONF_MSG_SER_REQ:
			r = config_get_serial(dynamic_cast<UsbProConfMsgSerReq*>(m));
		case LLA_USBPRO_CONF_MSG_SPRM_REQ:
			r = config_set_params(dynamic_cast<UsbProConfMsgSprmReq*>(m));
		default:
			Logger::instance()->log(Logger::WARN ,"Invalid request to usbpro configure %i", m->type());
			return NULL;
	}

	if (r != NULL ) {
		r->pack(repl);
		delete r;
	}
	return reply;
}

/*
 * Called when the dmx changes
 */
void UsbProDevice::new_dmx() {
	// notify our port
	Port *prt = get_port(0);
	prt->dmx_changed();
}


/*
 * Handle a get params request
 */
UsbProConfMsg *UsbProDevice::config_get_params(UsbProConfMsgPrmReq *req) const {
	UsbProConfMsgPrmRep *r = new UsbProConfMsgPrmRep();
	uint16_t firmware;
	uint8_t brk, mab, rate;
	m_widget->get_params(&firmware, &brk, &mab, &rate);
	r->set_firmware(firmware);
	r->set_brk(brk);
	r->set_mab(mab);
	r->set_rate(rate);
	return r;
}


/*
 * Handle a serial number request
 */
UsbProConfMsg *UsbProDevice::config_get_serial(UsbProConfMsgSerReq *req) const {
	UsbProConfMsgSerRep *r = new UsbProConfMsgSerRep();
	uint8_t serial[4];
	m_widget->get_serial(serial,4);
	r->set_serial(serial, 4);
	return r;
}


/*
 * Handle a set param config request
 */
UsbProConfMsg *UsbProDevice::config_set_params(UsbProConfMsgSprmReq *req) {
	UsbProConfMsgSprmRep *r = new UsbProConfMsgSprmRep();
	
	int ret = m_widget->set_params(NULL, 0, req->get_brk(), req->get_mab(), req->get_rate());
	r->set_status(ret);
	return r;
}