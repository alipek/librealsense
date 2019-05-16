// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-device.h"

#include <vector>
#include <algorithm>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class command_transfer
        {
        public:
            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) = 0;

            virtual ~command_transfer() = default;
        };

        class command_transfer_usb : public command_transfer
        {
        public:
            command_transfer_usb(const rs_usb_device& device) : _device(device) {}
            ~command_transfer_usb() {};

            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms,
                bool require_response = true) override
            { 
                const auto& m = _device->open();
                auto intfs = _device->get_interfaces();
                auto it = std::find_if(intfs.begin(), intfs.end(),
                    [](const rs_usb_interface& i) { return i->get_class() == RS2_USB_CLASS_VENDOR_SPECIFIC; });
                if (it == intfs.end())
                    throw std::runtime_error("can't find VENDOR_SPECIFIC interface of device: " + _device->get_info().id);

                auto hwm = *it;
                uint32_t transfered_count = 0;
                auto sts = m->bulk_transfer(hwm->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_WRITE), const_cast<uint8_t*>(data.data()), data.size(), transfered_count, timeout_ms);

                if (sts != RS2_USB_STATUS_SUCCESS)
                    throw std::runtime_error("command transfer failed to execute bulk transfer, error: " + usb_status_to_string.at(sts));

                std::vector<uint8_t> output(DEFAULT_BUFFER_SIZE);
                sts = m->bulk_transfer(hwm->first_endpoint(RS2_USB_ENDPOINT_DIRECTION_READ), output.data(), output.size(), transfered_count, timeout_ms);

                if (sts != RS2_USB_STATUS_SUCCESS)
                    throw std::runtime_error("command transfer failed to execute bulk transfer, error: " + usb_status_to_string.at(sts));

                output.resize(transfered_count);
                return output;
            }

        private:
            rs_usb_device _device;
            static const uint32_t DEFAULT_BUFFER_SIZE = 1024;
        };
    }
}