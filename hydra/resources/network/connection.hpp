//
// created by : Timothée Feuillet
// date: 2023-9-30
//
//
// Copyright (c) 2023 Timothée Feuillet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include "protocol.hpp"

#include <ntools/io/network_helper.hpp>
#include <ntools/io/connections.hpp>

namespace neam::resources::network
{
  /// \brief The connection contains the state
  struct connection_t : public io::network::header_connection_t<connection_t, 3u * 1024 * 1024 * 1024 /* limit the data to 3gib */>
  {
    using packet_header_t = network::packet_header_t;

    // Limits (can be lowered dynamically, but never increased past those values)
    static constexpr uint32_t k_max_queued_request_count = 16384;
    uint32_t max_queued_request_count = k_max_queued_request_count;
    static constexpr uint32_t k_max_active_request_count = 64;
    uint32_t max_active_request_count = k_max_active_request_count;

    uint32_t activity_timeout_seconds = 2; // kill the connection after this number of seconds

    using response_chain_t = async::chain<packet_header_t, raw_data&&/* packet_data*/>;
    struct queued_request_t
    {
      packet_header_t header;
      raw_data data;
    };
    std::unordered_map<uint16_t, response_chain_t::state> requests_awaiting_response;

    spinlock received_requests_lock;
    std::unordered_map<uint16_t, queued_request_t> received_requests;
    // cr::queue_ts<cr::queue_ts_wrapper<queued_request_t>> received_requests;

    void on_error(std::string&& message) const
    {
      cr::out().error("neam::resources::network::connection {}: {}", socket, std::move(message));
    }

    bool is_header_valid(const packet_header_t& ph) const
    {
      // not a valid header: bad magic
      if (ph.magic != (packet_header_t::k_magic ^ packet_header_t::k_version))
      {
        on_error("malformed packet header: bad magic");
        return false;
      }
      // not a valid header: neither a request nor a response
      if ((ph.code & code_t::rr_mask) == code_t::none)
      {
        on_error("malformed packet header: packet is neither a request nor a response");
        return false;
      }
      // check if the command is valid:
      if (!is_command_valid(ph.command, ph.code))
      {
        on_error("malformed packet header: command is not valid");
        return false;
      }
      // compute minimal size for packet, and check if they match:
      if (packet_size(ph.command, ph.code) - sizeof(packet_header_t) > ph.size)
      {
        on_error("malformed packet header: packet size is below the minimal expected for this command/code");
        return false;
      }
      // we can accept the packet
      return true;
    }
    static uint32_t get_size_of_data_to_read(const packet_header_t& ph) { return ph.size; }

    void on_packet(const packet_header_t& ph, raw_data&& packet_data)
    {
      // Handle responses asap:
      if ((ph.code & code_t::rr_mask) == code_t::response)
      {
        if (auto it = requests_awaiting_response.find(ph.command_id); it != requests_awaiting_response.end())
        {
          if (!it->second.is_canceled())
            it->second(ph, std::move(packet_data));
          requests_awaiting_response.erase(it);
        }
      }
      else // otherwise: queue requests
      {
        // except for cancel requests. It cost less to handle them immediately
        if (ph.command == command_t::cancel_request)
        {
          // we still double check the size, just in case "something" happens
          if (packet_data.size == sizeof(commands::req_cancel_request_t))
          {
            const uint16_t command_id = packet_data.get_as<commands::req_cancel_request_t>()->request_id;

            std::lock_guard _lg{received_requests_lock};
            {
              received_requests.erase(command_id);
              // FIXME: cancel in-flight requests (only read requests, only if possible)
            }
          }
          else
          {
            on_error("malformed cancel request (data too small)");
            close();
            return;
          }
        }
        else
        {
          std::lock_guard _lg{received_requests_lock};
          if (received_requests.contains(ph.command_id))
          {
            // non recoverable error, as those requests are now indistinguishable
            on_error("malformed request: duplicate command id found");
            close();
            return;
          }
          if (received_requests.size() > max_queued_request_count || received_requests.size() > k_max_queued_request_count)
          {
            // non recoverable error, too many requests
            on_error("received_requests size is above the set maximum");
            close();
            return;
          }

          received_requests.emplace(ph.command_id, queued_request_t{ph, std::move(packet_data)});
        }
      }
    }

    void process_single_request()
    {
      queued_request_t rq;
      {
        std::lock_guard _lg{received_requests_lock};
        if (received_requests.empty()) return;

        rq = std::move(received_requests.begin()->second);
        received_requests.erase(received_requests.begin());
      }

      // FIXME: do the thing
    }
  };
}

