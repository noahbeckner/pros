/**
 * \file vdml_link.c
 *
 * \brief Contains source code for functions related to the robot to robot communications.
 *
 * Visit https://pros.cs.purdue.edu/v5/api/c/link.html to learn
 * more.
 *
 * This file should not be modified by users, since it gets replaced whenever
 * a kernel upgrade occurs.
 *
 * Copyright (c) 2017-2022, Purdue University ACM SIGBots.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

 #include "pros/link.h"
 #include "kapi.h"
 #include "pros/apix.h"
 #include "vdml/vdml.h"
 #include <string.h>

#define FIFO_SIZE           512 // VEX Defined TX/RX Queue Size
#define PROTOCOL_SIZE         4 // Protocol Size 

static uint8_t start_byte = 0x33;

// internal function for clearing the rx buffer 
uint32_t _clear_rx_buf(v5_smart_device_s_t* device) {
    uint8_t buf[FIFO_SIZE];
    return vexDeviceGenericRadioReceive(device->device_info, 
    (uint8_t*)buf, 
    vexDeviceGenericRadioReceiveAvail(device->device_info));
}

// internal wrapper for initialization
inline uint32_t _link_init_wrapper(uint8_t port, char* link_id, link_type_e_t type, bool ov) {
    // not used because it's a generic serial device 
    // claim_port_i(port - 1, E_DEVICE_RADIO);
	if (!VALIDATE_PORT_NO(port - 1)) {
		errno = EINVAL;
		return PROS_ERR;
	}
    // by default, the vexlink radio in the lower port should be by default E_DEVICE_NONE
    // or, the only radio on the brain
    v5_device_e_t plugged_device = registry_get_plugged_type(port - 1);
    if(plugged_device != E_DEVICE_NONE && plugged_device != E_DEVICE_RADIO) {
        return PROS_ERR;
    }
	v5_smart_device_s_t* device = registry_get_device(port - 1);
	if (!port_mutex_take(port - 1)) {
		errno = EACCES;
		return PROS_ERR;
	}
    vexDeviceGenericRadioConnection(device->device_info, link_id, type, ov);
    return_port(port - 1, 1);
}

uint32_t link_init(uint8_t port, char* link_id, link_type_e_t type) {
    return _link_init_wrapper(port, link_id, type, false);
}

uint32_t link_init_override(uint8_t port, char* link_id, link_type_e_t type) {
    return _link_init_wrapper(port, link_id, type, true);
}

bool link_connected(uint8_t port) {
    claim_port(port - 1, E_DEVICE_SERIAL, false);
    bool rtv = vexDeviceGenericRadioLinkStatus(device->device_info); 
    return_port(port - 1, rtv);
}

uint32_t link_raw_receivable_size(uint8_t port) {
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    uint32_t rtv = vexDeviceGenericRadioReceiveAvail(device->device_info); 
    return_port(port - 1, rtv);
}

uint32_t link_raw_transmittable_size(uint8_t port) {
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    uint32_t rtv = vexDeviceGenericRadioWriteFree(device->device_info);
    return_port(port - 1, rtv);
}

uint32_t link_transmit_raw(uint8_t port, void* data, uint16_t data_size) {
    if(data == NULL) {
        errno = EINVAL;
        return PROS_ERR;
    }
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    if(!vexDeviceGenericRadioLinkStatus(device->device_info)) {
        errno = ENXIO;
        return_port(port - 1, PROS_ERR);
    }
    if(data_size > vexDeviceGenericRadioWriteFree(device->device_info)) {
        errno = EBUSY;
        return_port(port - 1, 0);
    }
    uint32_t vexDeviceGenericRadioTransmit(device->device_info, (uint8_t*)data, data_size);
    return_port(port - 1, rtv);
}

uint32_t link_receive_raw(uint8_t port, void* dest, uint16_t data_size) {
    claim_port_i(port - 1, E_DEVICE_SERIAL); 
    if(data_size > vexDeviceGenericRadioReceiveAvail(device->device_info)) {
        errno = EINVAL;
        return_port(port - 1, 0);
    }
    if(!vexDeviceGenericRadioLinkStatus(device->device_info)) {
        errno = ENXIO;
        return_port(port - 1, PROS_ERR);
    }
    if(dest == NULL) {
        errno = EINVAL;
        return_port(port - 1, PROS_ERR);
    }
    uint32_t rtv = vexDeviceGenericRadioReceive(device->device_info, (uint8_t*)dest, data_size);
    return_port(port - 1, rtv);
}

uint32_t link_transmit(uint8_t port, void* data, uint16_t data_size) {
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    if(!vexDeviceGenericRadioLinkStatus(device->device_info)) {
        errno = ENXIO;
        return_port(port - 1, PROS_ERR);
    }
    if(data_size + PROTOCOL_SIZE > vexDeviceGenericRadioWriteFree(device->device_info)) {
        errno = EBUSY;
        kprintf("Data Size too big: %d\n", data_size + PROTOCOL_SIZE);
        return_port(port - 1, 0);
    }
    if(data == NULL) {
        errno = EINVAL;
        return_port(port - 1, PROS_ERR);
    }
    // calculated checksum
    uint8_t checksum = start_byte;
    uint8_t size_tx_buf[2];
    size_tx_buf[1] = (data_size >> 8) & 0xff;
    size_tx_buf[0] = (data_size) & 0xff;
    checksum ^= size_tx_buf[1];
    checksum ^= size_tx_buf[0];
    
    for(int i = 0; i < data_size; i++) {
        checksum ^= ((uint8_t*)data)[i];
    }

    // send protocol
    vexDeviceGenericRadioTransmit(device->device_info, &start_byte, 1);
    vexDeviceGenericRadioTransmit(device->device_info, size_tx_buf, 2);
    vexDeviceGenericRadioTransmit(device->device_info, (uint8_t*)data, data_size);
    vexDeviceGenericRadioTransmit(device->device_info, &checksum, 1);
    return_port(port - 1, 1);
}

uint32_t link_receive(uint8_t port, void* data, uint16_t data_size) {
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    if(!vexDeviceGenericRadioLinkStatus(device->device_info)) {
        errno = ENXIO;
        return_port(port - 1, PROS_ERR);
    }
    if(data == NULL) {
        errno = EINVAL;
        return_port(port - 1, PROS_ERR);
    }
    if(data_size + PROTOCOL_SIZE > vexDeviceGenericRadioReceiveAvail(device->device_info)) {
        _clear_rx_buf(device);
        errno = EBUSY;
        return_port(port - 1, 0);
    }
    uint8_t header_byte;
    uint8_t received_size;
    // process protocol
    received_size = vexDeviceGenericRadioReceive(device->device_info, &header_byte, 1); // 0x33
    if(start_byte != header_byte || received_size != 1) {
        kprintf("[VEXLINK] Invalid Header Byte Received Port %d, header byte: %x", port, header_byte);
        errno = EBADMSG;
        return_port(port - 1, PROS_ERR);
    }
    // datasize
    uint16_t _size;
    received_size = vexDeviceGenericRadioReceive(device->device_info, (uint8_t*)&_size, 2);
    if(received_size != 2 || _size != data_size) {
        _clear_rx_buf(device);
        kprintf("[VEXLINK] Invalid Data Size (Size: %d ) Received Port %d, flushing RX buffer!\n", _size, port);
        errno = EBADMSG;
        return_port(port - 1, PROS_ERR);
    }
    // receive data
    uint32_t rtv;
    rtv = vexDeviceGenericRadioReceive(device->device_info, (uint8_t*)data, data_size);
    if(rtv != data_size || _size != data_size) {
        kprintf("[VEXLINK] Invalid Data Received Port %d, flushing RX buffer!\n", port);
        errno = EBADMSG;
        _clear_rx_buf(device);
    }
    uint8_t received_checksum;
    received_size = vexDeviceGenericRadioReceive(device->device_info, &received_checksum, 1);
    // check checksum
    uint8_t calculated_checksum = start_byte;
    calculated_checksum ^= (data_size >> 8) & 0xff;
    calculated_checksum ^= (data_size) & 0xff;
    for(int i = 0; i < data_size; i++) {
        calculated_checksum ^= ((uint8_t*)data)[i];
    }
    if(calculated_checksum != received_checksum || received_size != 1) {
        kprintf("[VEXLINK] Checksum Mismatch Port %d!, Checksum: %x\n", port, received_checksum);
        errno = EBADMSG;
        return_port(port - 1, PROS_ERR);
    }
    return_port(port - 1, rtv);
}

uint32_t link_clear_receive_buf(uint8_t port) {
    claim_port_i(port - 1, E_DEVICE_SERIAL);
    uint32_t rtv = _clear_rx_buf(device);
    return_port(port - 1, rtv);
}

