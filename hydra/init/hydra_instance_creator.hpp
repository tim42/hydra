//
// file : hydra_instance_creator.hpp
// in : file:///home/tim/projects/hydra/hydra/init/hydra_instance_creator.hpp
//
// created by : Timothée Feuillet
// date: Mon Apr 25 2016 18:10:48 GMT+0200 (CEST)
//
//
// Copyright (c) 2016 Timothée Feuillet
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

#ifndef __N_258106574877730109_846630854_HYDRA_INSTANCE_CREATOR_HPP__
#define __N_258106574877730109_846630854_HYDRA_INSTANCE_CREATOR_HPP__

#include <initializer_list>
#include <vector>
#include <set>
#include <map>

#include <vulkan/vulkan.h>

#include "layer.hpp"
#include "extension.hpp"
#include "feature_requester_interface.hpp"
#include "hydra_vulkan_instance.hpp"

namespace neam
{
  namespace hydra
  {
    /// \brief Provide a mean to initialize vulkan and hydra
    /// \note This is quite low level, you may find a way to avoid this in a
    ///       higher level interface.
    class hydra_instance_creator
    {
      public: // static interface
        /// \brief Get the list of instance layers
        static std::vector<vk_layer> get_instance_layers()
        {
          std::vector<VkLayerProperties> vk_layer_list;
          VkResult res;
          uint32_t instance_layer_count;
          do
          {
            res = check::on_vulkan_error::n_throw_exception(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

            if (instance_layer_count == 0)
              break;
            vk_layer_list.resize(instance_layer_count);
            res = check::on_vulkan_error::n_throw_exception(vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_layer_list.data()));
          }
          while (res == VK_INCOMPLETE);

          std::vector<vk_layer> result_list;
          result_list.reserve(vk_layer_list.size());

          for (const auto &it : vk_layer_list)
            result_list.push_back(vk_layer(it));

          return result_list;
        }

        /// \brief Retrieve the instance extensions
        static std::vector<vk_extension> get_instance_extensions()
        {
          std::vector<VkExtensionProperties> vk_ext_list;
          VkResult res;
          uint32_t instance_layer_count;
          do
          {
            res = check::on_vulkan_error::n_throw_exception(vkEnumerateInstanceExtensionProperties(NULL, &instance_layer_count, nullptr));

            if (instance_layer_count == 0)
              break;
            vk_ext_list.resize(instance_layer_count);
            res = check::on_vulkan_error::n_throw_exception(vkEnumerateInstanceExtensionProperties(NULL, &instance_layer_count, vk_ext_list.data()));
          }
          while (res == VK_INCOMPLETE);

          std::vector<vk_extension> result_list;
          result_list.reserve(vk_ext_list.size());

          for (const auto &it : vk_ext_list)
            result_list.push_back(vk_extension(it));

          return result_list;
        }

      public: // members
        /// \brief Default initialize all the fields of the application information
        hydra_instance_creator() : hydra_instance_creator("hydra_application", 1) {}
        /// \brief Constructor that let you define the fields of the app name and version
        explicit hydra_instance_creator(const std::string &_app_name, size_t _app_version = 1)
         : app_name(_app_name), app_version(_app_version), engine_name("hydra"), engine_version(1) // TODO: handle the version a bit better
        { setup_lists(); }
        /// \brief Constructor that let you define the app information and part of the engine information
        hydra_instance_creator(const std::string &_app_name, size_t _app_version, const std::string &_engine_name, size_t _engine_version)
         : app_name(_app_name), app_version(_app_version), engine_name("hydra/" + _engine_name), engine_version(_engine_version)
        { setup_lists(); }

        /// \brief Destructor
        ~hydra_instance_creator() {}

        /// \brief Set the version of vulkan required by the application
        /// (the default version is a correct version)
        void set_vulkan_api_version(uint32_t _vulkan_api_version) { vulkan_api_version = _vulkan_api_version; }

        /// \brief Set the version of vulkan required by the application
        /// (the default version is a correct version)
        void set_vulkan_api_version(uint32_t _vulkan_api_version_major, uint32_t _vulkan_api_version_minor, uint32_t _vulkan_api_version_patch)
        {
          vulkan_api_version = VK_MAKE_VERSION(_vulkan_api_version_major, _vulkan_api_version_minor, _vulkan_api_version_patch);
        }

        /// \brief Return the vulkan API version used by the current instance
        uint32_t get_vulkan_api_version() const { return vulkan_api_version; }

        /// \brief Query the application name
        std::string get_application_name() const { return app_name; }
        /// \brief Query the application version
        size_t get_application_version() const { return app_version; }
        /// \brief Query the engine name
        std::string get_engine_name() const { return engine_name; }
        /// \brief Query the engine version
        size_t get_engine_version() const { return engine_version; }

        /// \brief Check if a given extension is provided by vulkan
        bool have_extension(const std::string &extension_name) const { return instance_extension_list.count(extension_name); }
        /// \brief Check if a given layer is provided by vulkan
        bool have_layer(const std::string &layer_name) const { return instance_layer_list.count(layer_name); }

        /// \brief Require an extension for the instance
        /// \throw neam::hydra::exception if the extension does not exists
        void require_extension(const std::string &extension_name)
        {
          if (!have_extension(extension_name))
            throw exception_tpl<hydra_instance_creator>("Required extension is not provided by vulkan", __FILE__, __LINE__);
          if (!instance_extensions.count(extension_name))
            instance_extensions.emplace(extension_name);
        }
        /// \brief Require a list of extension for the instance
        /// \throw neam::hydra::exception if any of those extension does not exists
        void require_extensions(std::initializer_list<std::string> extension_names)
        {
          for (const auto &it : extension_names)
            require_extension(it);
        }

        /// \brief Require a layer for the instance
        /// \throw neam::hydra::exception if the layer does not exists
        void require_layer(const std::string &layer_name)
        {
          if (!have_layer(layer_name))
            throw exception_tpl<hydra_instance_creator>("Required layer is not provided by vulkan", __FILE__, __LINE__);
          if (!instance_layers.count(layer_name))
            instance_layers.emplace(layer_name);
        }

        /// \brief Require a list of layer for the instance
        /// \throw neam::hydra::exception if any of those layer does not exists
        void require_layers(std::initializer_list<std::string> layer_names)
        {
          for (const auto &it : layer_names)
            require_layer(it);
        }

        /// \brief Let a requester ask for a list of extension and layers
        /// A requester \e should have the following method:
        ///   void request_instace_layers_extensions(hydra_instance_creator &)
        /// \see feature_requester_interface
        template<typename RequesterT>
        void require(RequesterT requester)
        {
          requester.request_instace_layers_extensions(*this);
        }

        /// \brief Like require<>, but let you work with an abstract type
        void require(feature_requester_interface *requester)
        {
          require<feature_requester_interface &>(*requester);
        }

        /// \brief Like require<>, but let you work with an abstract type
        void require(feature_requester_interface &requester)
        {
          require<feature_requester_interface &>(requester);
        }

        /// \brief Create the hydra/vulkan instance
        hydra_vulkan_instance create_instance() const
        {
          VkApplicationInfo app_info = {};
          app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
          app_info.pNext = nullptr;
          app_info.pApplicationName = app_name.c_str();
          app_info.applicationVersion = 1;
          app_info.pEngineName = engine_name.c_str();
          app_info.engineVersion = 1;
          app_info.apiVersion = vulkan_api_version;

          std::vector<const char *> instance_layers_c_str;
          instance_layers_c_str.reserve(instance_layers.size());
          for (const std::string &it : instance_layers)
            instance_layers_c_str.push_back(it.c_str());

          std::vector<const char *> instance_extensions_c_str;
          instance_extensions_c_str.reserve(instance_extensions.size());
          for (const std::string &it : instance_extensions)
            instance_extensions_c_str.push_back(it.c_str());

          VkInstanceCreateInfo inst_info = {};
          inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
          inst_info.pNext = nullptr;
          inst_info.flags = 0;
          inst_info.pApplicationInfo = &app_info;
          inst_info.enabledLayerCount = instance_layers.size();
          inst_info.ppEnabledLayerNames = instance_layers.size()
                                          ? instance_layers_c_str.data()
                                          : nullptr;
          inst_info.enabledExtensionCount = instance_extensions.size();
          inst_info.ppEnabledExtensionNames = instance_extensions.size()
                                              ? instance_extensions_c_str.data()
                                              : nullptr;

          VkInstance vk_inst;
          check::on_vulkan_error::n_throw_exception(vkCreateInstance(&inst_info, nullptr, &vk_inst));

          return hydra_vulkan_instance(vk_inst, app_name);
        }

      private: // members
        void setup_lists()
        {
          // construct the std::map<> from the results of the different lists
          auto layers = get_instance_layers();
          for (const vk_layer &it : layers)
            instance_layer_list.emplace(it.get_name(), it);
          auto extensions = get_instance_extensions();
          for (const vk_extension &it : extensions)
            instance_extension_list.emplace(it.get_name(), it);
        }

      private: // priv. attributes
        std::string app_name;
        size_t app_version;
        std::string engine_name;
        size_t engine_version;
        uint32_t vulkan_api_version = VK_MAKE_VERSION(1, 0, 0);

        std::set<std::string> instance_layers;
        std::set<std::string> instance_extensions;

        std::map<std::string, vk_layer> instance_layer_list;
        std::map<std::string, vk_extension> instance_extension_list;
    };
  } // namespace hydra
} // namespace neam

#endif // __N_258106574877730109_846630854_HYDRA_INSTANCE_CREATOR_HPP__

