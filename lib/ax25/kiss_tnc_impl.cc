/*
 * Copyright 2024 gr-packet-protocols
 *
 * This file is part of gr-packet-protocols
 *
 * gr-packet-protocols is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * gr-packet-protocols is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gr-packet-protocols; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "kiss_tnc_impl.h"
#include <fcntl.h>
#include <gnuradio/io_signature.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace gr {
namespace packet_protocols {

kiss_tnc::sptr kiss_tnc::make(const std::string& device, int baud_rate,
                              bool hardware_flow_control) {
    return gnuradio::make_block_sptr<kiss_tnc_impl>(device, baud_rate, hardware_flow_control);
}

kiss_tnc_impl::kiss_tnc_impl(const std::string& device, int baud_rate, bool hardware_flow_control)
    : gr::sync_block("kiss_tnc", gr::io_signature::make(1, 1, sizeof(char)),
                     gr::io_signature::make(1, 1, sizeof(char))),
      d_device(device), d_baud_rate(baud_rate), d_hardware_flow_control(hardware_flow_control),
      d_serial_fd(-1), d_kiss_state(KISS_STATE_IDLE), d_escape_next(false), d_frame_buffer(1024),
      d_frame_length(0), d_ones_count(0) {
    // Initialize serial port
    if (!open_serial_port()) {
        throw std::runtime_error("Failed to open serial port: " + device);
    }

    d_frame_buffer.clear();
    d_frame_length = 0;
}

kiss_tnc_impl::~kiss_tnc_impl() {
    if (d_serial_fd >= 0) {
        close(d_serial_fd);
    }
}

bool kiss_tnc_impl::open_serial_port() {
    d_serial_fd = open(d_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (d_serial_fd < 0) {
        return false;
    }

    // Configure serial port
    struct termios tty;
    if (tcgetattr(d_serial_fd, &tty) != 0) {
        close(d_serial_fd);
        d_serial_fd = -1;
        return false;
    }

    // Set baud rate
    speed_t speed = B9600;
    switch (d_baud_rate) {
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        break;
    }

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Configure 8N1
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore modem control lines

    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;

    // Set timeouts
    tty.c_cc[VTIME] = 0; // No timeout
    tty.c_cc[VMIN] = 0;  // Non-blocking read

    if (tcsetattr(d_serial_fd, TCSANOW, &tty) != 0) {
        close(d_serial_fd);
        d_serial_fd = -1;
        return false;
    }

    return true;
}

int kiss_tnc_impl::work(int noutput_items, gr_vector_const_void_star& input_items,
                        gr_vector_void_star& output_items) {
    const char* in = (const char*)input_items[0];
    char* out = (char*)output_items[0];

    int consumed = 0;
    int produced = 0;

    // Process input from GNU Radio
    for (int i = 0; i < noutput_items; i++) {
        if (d_kiss_state == KISS_STATE_IDLE && in[i] == KISS_FEND) {
            d_kiss_state = KISS_STATE_FRAME;
            d_frame_buffer.clear();
            d_frame_length = 0;
            d_escape_next = false;
        } else if (d_kiss_state == KISS_STATE_FRAME) {
            if (in[i] == KISS_FEND) {
                // End of frame
                if (d_frame_length > 0) {
                    process_kiss_frame();
                }
                d_kiss_state = KISS_STATE_IDLE;
            } else if (in[i] == KISS_FESC) {
                d_escape_next = true;
            } else {
                uint8_t byte = in[i];
                if (d_escape_next) {
                    if (byte == KISS_TFESC) {
                        byte = KISS_FESC;
                    } else if (byte == KISS_TFEND) {
                        byte = KISS_FEND;
                    }
                    d_escape_next = false;
                }

                if (d_frame_length < d_frame_buffer.size()) {
                    d_frame_buffer[d_frame_length] = byte;
                    d_frame_length++;
                }
            }
        }
        consumed++;
    }

    // Read from serial port and output to GNU Radio
    if (d_serial_fd >= 0) {
        char serial_buffer[256];
        int bytes_read = read(d_serial_fd, serial_buffer, sizeof(serial_buffer));

        if (bytes_read > 0) {
            for (int i = 0; i < bytes_read && produced < noutput_items; i++) {
                out[produced] = serial_buffer[i];
                produced++;
            }
        }
    }

    return produced;
}

void kiss_tnc_impl::process_kiss_frame() {
    if (d_frame_length < 2) {
        return; // Invalid frame
    }

    uint8_t command = d_frame_buffer[0] & 0x0F;
    uint8_t port = (d_frame_buffer[0] >> 4) & 0x0F;

    switch (command) {
    case KISS_CMD_DATA:
        // Data frame - send to serial port
        if (d_serial_fd >= 0) {
            write(d_serial_fd, &d_frame_buffer[1], d_frame_length - 1);
        }
        break;

    case KISS_CMD_TXDELAY:
        // Set TX delay
        if (d_frame_length >= 2) {
            d_tx_delay = d_frame_buffer[1];
        }
        break;

    case KISS_CMD_P:
        // Set persistence
        if (d_frame_length >= 2) {
            d_persistence = d_frame_buffer[1];
        }
        break;

    case KISS_CMD_SLOTTIME:
        // Set slot time
        if (d_frame_length >= 2) {
            d_slot_time = d_frame_buffer[1];
        }
        break;

    case KISS_CMD_TXTAIL:
        // Set TX tail
        if (d_frame_length >= 2) {
            d_tx_tail = d_frame_buffer[1];
        }
        break;

    case KISS_CMD_FULLDUPLEX:
        // Set full duplex mode
        if (d_frame_length >= 2) {
            d_full_duplex = (d_frame_buffer[1] != 0);
        }
        break;

    case KISS_CMD_SET_HARDWARE:
        // Set hardware parameters
        if (d_frame_length >= 2) {
            d_hardware_type = d_frame_buffer[1];
        }
        break;

    case KISS_CMD_RETURN:
        // Return to normal mode
        d_kiss_mode = false;
        break;
    }
}

void kiss_tnc_impl::send_kiss_frame(uint8_t command, uint8_t port, const uint8_t* data,
                                    int length) {
    if (d_serial_fd < 0) {
        return;
    }

    // Build KISS frame
    std::vector<uint8_t> frame;
    frame.push_back(KISS_FEND);
    frame.push_back((port << 4) | command);

    // Add data with escaping
    for (int i = 0; i < length; i++) {
        if (data[i] == KISS_FEND) {
            frame.push_back(KISS_FESC);
            frame.push_back(KISS_TFEND);
        } else if (data[i] == KISS_FESC) {
            frame.push_back(KISS_FESC);
            frame.push_back(KISS_TFESC);
        } else {
            frame.push_back(data[i]);
        }
    }

    frame.push_back(KISS_FEND);

    // Send frame
    write(d_serial_fd, frame.data(), frame.size());
}

void kiss_tnc_impl::set_tx_delay(int delay) {
    d_tx_delay = delay;
    uint8_t cmd_data = delay & 0xFF;
    send_kiss_frame(KISS_CMD_TXDELAY, 0, &cmd_data, 1);
}

void kiss_tnc_impl::set_persistence(int persistence) {
    d_persistence = persistence;
    uint8_t cmd_data = persistence & 0xFF;
    send_kiss_frame(KISS_CMD_P, 0, &cmd_data, 1);
}

void kiss_tnc_impl::set_slot_time(int slot_time) {
    d_slot_time = slot_time;
    uint8_t cmd_data = slot_time & 0xFF;
    send_kiss_frame(KISS_CMD_SLOTTIME, 0, &cmd_data, 1);
}

void kiss_tnc_impl::set_tx_tail(int tx_tail) {
    d_tx_tail = tx_tail;
    uint8_t cmd_data = tx_tail & 0xFF;
    send_kiss_frame(KISS_CMD_TXTAIL, 0, &cmd_data, 1);
}

void kiss_tnc_impl::set_full_duplex(bool full_duplex) {
    d_full_duplex = full_duplex;
    uint8_t cmd_data = full_duplex ? 1 : 0;
    send_kiss_frame(KISS_CMD_FULLDUPLEX, 0, &cmd_data, 1);
}

} /* namespace packet_protocols */
} /* namespace gr */
