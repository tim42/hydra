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

#include <ntools/id/id.hpp>
#include <ntools/raw_data.hpp>
#include <ntools/enum.hpp>
#include <ntools/rle/rle.hpp>

namespace neam::resources::network
{
  enum class command_t : uint8_t
  {
    //
    // client -> server initiated commands
    //

    hello, // if not present in the first second of the connection being initiated, the server closes the connection

    has_resource, // simple true/false reply,
    // NOTE: compressed resources are left compressed (only replace the io call). Decompression is left to the client
    read_raw_resource,
    read_source_file, // perform a read operation on a file in the source folder, if there's such a folder

    get_reldb, // send-back the reldb, if the server has one

    //
    // server -> client initiated commands
    //
    index_reloaded_event, // just header, no response needed

    //
    // management:
    //

    // cancel a request.
    // No response is sent for this operation, and a response for it can still get sent.
    // If the command is actually canceled, no response for it will be sent. Otherwise, a response can still get sent
    // NOTE: not all requests support cancelation
    // This command doesn't need a command id and will be treated as soon as it arrives
    cancel_request,
  };
  enum class code_t : uint8_t
  {
    none = 0,

    request = 1,
    response = 2,
    rr_mask = 0x3,

    has_error = 1 << 4, // if the reply contains an error packet
    has_data = 1 << 5, // if the reply contains a data packet
  };
  N_ENUM_FLAG(code_t)

  enum class error_code_t : uint8_t
  {
    none,
    not_found,
  };

  struct packet_header_t
  {
    static constexpr uint32_t k_magic = 0x4F5A3B00;
    static constexpr uint32_t k_version = 0x01;

    uint32_t magic;
    uint32_t size;

    command_t command;
    code_t code;

    // used to associate replies to requests
    uint16_t command_id;
  };

  struct error_response_t
  {
    error_code_t error_code;
  };

  namespace internal
  {
    struct empty_t {};

    template<typename Request, typename Response>
    struct rr_pait_t
    {
      using request_t = Request;
      using response_t = Response;
    };
  }

  namespace commands
  {
    template<command_t> struct rr_command_types_t : public internal::rr_pait_t<internal::empty_t, internal::empty_t> {};

    template<command_t C> using request_t = typename rr_command_types_t<C>::request_t;
    template<command_t C> using response_t = typename rr_command_types_t<C>::response_t;

    template<> struct rr_command_types_t<command_t::hello> : public internal::rr_pait_t<internal::empty_t, void> {};
    template<> struct rr_command_types_t<command_t::index_reloaded_event> : public internal::rr_pait_t<internal::empty_t, void> {};

    struct req_has_resource_t
    {
      id_t resource;
    };
    struct res_has_resource_t
    {
      bool has_resource;
    };
    template<> struct rr_command_types_t<command_t::has_resource> : public internal::rr_pait_t<req_has_resource_t, res_has_resource_t> {};

    struct req_read_raw_resource_t
    {
      id_t resource;
    };
    struct res_read_raw_resource_t
    {
      bool is_compressed;

      uint8_t resource_data[];
    };
    template<> struct rr_command_types_t<command_t::read_raw_resource> : public internal::rr_pait_t<req_read_raw_resource_t, res_read_raw_resource_t> {};

    struct req_read_source_file_t
    {
      id_t file;
    };

    template<> struct rr_command_types_t<command_t::read_source_file> : public internal::rr_pait_t<req_read_source_file_t, internal::empty_t> {};
    template<> struct rr_command_types_t<command_t::get_reldb> : public internal::rr_pait_t<internal::empty_t, internal::empty_t> {};

    struct req_cancel_request_t
    {
      uint16_t request_id;
    };
    template<> struct rr_command_types_t<command_t::cancel_request> : public internal::rr_pait_t<req_cancel_request_t, void /* invalid, no reply for cancel */> {};
  }


  template<command_t Command> constexpr size_t packet_size(code_t code)
  {
    size_t ret = sizeof(packet_header_t);
    if ((code & code_t::has_data) == code_t::has_data)
    {
      if ((code & code_t::rr_mask) == code_t::request)
      {
        if constexpr(!std::is_same_v<commands::request_t<Command>, internal::empty_t>)
          ret += sizeof(commands::request_t<Command>);
      }
      else if ((code & code_t::rr_mask) == code_t::response)
      {
        if constexpr(!std::is_same_v<commands::response_t<Command>, internal::empty_t>
                     && !std::is_same_v<commands::response_t<Command>, void>)
          ret += sizeof(commands::response_t<Command>);
      }
    }
    if ((code & code_t::has_error) == code_t::has_error)
      ret += sizeof(error_response_t);
    return ret;
  }

  constexpr size_t packet_size(command_t command, code_t code)
  {
    switch(command)
    {
      case command_t::hello: return packet_size<command_t::hello>(code);
      case command_t::has_resource: return packet_size<command_t::has_resource>(code);
      case command_t::read_raw_resource: return packet_size<command_t::read_raw_resource>(code);
      case command_t::read_source_file: return packet_size<command_t::read_source_file>(code);
      case command_t::get_reldb: return packet_size<command_t::get_reldb>(code);
      case command_t::index_reloaded_event: return packet_size<command_t::index_reloaded_event>(code);
      case command_t::cancel_request: return packet_size<command_t::cancel_request>(code);
    }
    return 0;
  }

  template<command_t Command> constexpr size_t is_command_valid(code_t code)
  {
    if ((code & code_t::has_data) == code_t::has_data)
    {
      if ((code & code_t::rr_mask) == code_t::request)
      {
        if constexpr(!std::is_same_v<commands::request_t<Command>, void>)
          return true;
        return false;
      }
      else if ((code & code_t::rr_mask) == code_t::response)
      {
        if constexpr(!std::is_same_v<commands::response_t<Command>, void>)
          return true;
        return false;
      }
      return false;
    }

    if ((code & code_t::has_error) == code_t::has_error)
      return true;

    return false;
  }

  constexpr bool is_command_valid(command_t command, code_t code)
  {
    switch(command)
    {
      case command_t::hello: return is_command_valid<command_t::hello>(code);
      case command_t::has_resource: return is_command_valid<command_t::has_resource>(code);
      case command_t::read_raw_resource: return is_command_valid<command_t::read_raw_resource>(code);
      case command_t::read_source_file: return is_command_valid<command_t::read_source_file>(code);
      case command_t::get_reldb: return is_command_valid<command_t::get_reldb>(code);
      case command_t::index_reloaded_event: return is_command_valid<command_t::index_reloaded_event>(code);
      case command_t::cancel_request: return is_command_valid<command_t::cancel_request>(code);
    }
    return false;
  }

  template<command_t Command, code_t Code = code_t::none>
  [[nodiscard]] raw_data form_packet(uint16_t command_id, uint32_t extra_data = 0)
  {
    raw_data rd = raw_data::allocate(packet_size<Command>(Code) + extra_data);
    packet_header_t* ph = rd.get_as<packet_header_t>();
    *ph =
    {
      .magic = packet_header_t::k_magic ^ packet_header_t::k_version,
      .size = (uint32_t)(rd.size - sizeof(packet_header_t)),
      .command = Command,
      .code = Code,
      .command_id = command_id,
    };
    return rd;
  }

  template<command_t Command>
  [[nodiscard]] raw_data form_packet(uint16_t command_id, commands::response_t<Command> response, raw_data&& extra_data = {})
  {
    static_assert(std::is_standard_layout_v<commands::response_t<Command>>, "Only standard layout types are supported here");

    raw_data rd = form_packet<Command, code_t::response | code_t::has_data>(command_id, (uint32_t)extra_data.size);
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t), &response, sizeof(response));
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t) + sizeof(response), extra_data.get(), extra_data.size);
    return rd;
  }

  template<command_t Command>
  [[nodiscard]] raw_data form_packet(uint16_t command_id, commands::request_t<Command> request, raw_data&& extra_data = {})
  {
    static_assert(std::is_standard_layout_v<commands::request_t<Command>>, "Only standard layout types are supported here");

    raw_data rd = form_packet<Command, code_t::request | code_t::has_data>(command_id, (uint32_t)extra_data.size);
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t), &request, sizeof(request));
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t) + sizeof(request), extra_data.get(), extra_data.size);
    return rd;
  }

  template<command_t Command>
  [[nodiscard]] raw_data form_packet(uint16_t command_id, error_response_t error)
  {
    static_assert(std::is_standard_layout_v<error_response_t>, "Only standard layout types are supported here");

    raw_data rd = form_packet<Command, code_t::response | code_t::has_error>(command_id);
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t), &error, sizeof(error));
    return rd;
  }

  template<command_t Command>
  [[nodiscard]] raw_data form_packet(uint16_t command_id, error_response_t error, commands::response_t<Command> response, raw_data&& extra_data = {})
  {
    static_assert(std::is_standard_layout_v<error_response_t>, "Only standard layout types are supported here");
    static_assert(std::is_standard_layout_v<commands::response_t<Command>>, "Only standard layout types are supported here");

    raw_data rd = form_packet<Command, code_t::response | code_t::has_data | code_t::has_error>(command_id, (uint32_t)extra_data.size);
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t), &error, sizeof(error));
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t) + sizeof(error), &response, sizeof(response));
    memcpy(rd.get_as<uint8_t>() + sizeof(packet_header_t) + sizeof(error) + sizeof(response), extra_data.get(), extra_data.size);
    return rd;
  }
}


