#ifndef Corrade_PluginManager_AbstractPluginManager_h
#define Corrade_PluginManager_AbstractPluginManager_h
/*
    Copyright © 2007, 2008, 2009, 2010, 2011, 2012
              Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Corrade.

    Corrade is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Corrade is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Corrade::PluginManager::AbstractPluginManager
 */

#include <vector>
#include <string>
#include <map>

#ifdef _WIN32
#include <windows.h>
#undef interface
#endif

#include "Utility/Resource.h"
#include "Utility/Debug.h"
#include "PluginMetadata.h"

namespace Corrade { namespace PluginManager {

class Plugin;

/**
 * @brief Non-templated base class of PluginManager
 *
 * Base abstract class for all PluginManager templated classes. See also
 * @ref plugin-management.
 */
class PLUGINMANAGER_EXPORT AbstractPluginManager {
    friend class Plugin;

    DISABLE_COPY(AbstractPluginManager)

    public:
        /** @brief Plugin instancer function */
        typedef void* (*Instancer)(AbstractPluginManager*, const std::string&);

        /**
         * @brief Load state
         *
         * Describes state of the plugin. States before Unknown state are used
         * when loading plugins, states after are used when unloading plugins.
         * Static plugins are loaded at first, they have always state
         * PluginMetadataStatic::IsStatic. Dynamic plugins have at first state
         * NotLoaded, after first attempt to load the state is changed.
         */
        enum LoadState {
            /** %Plugin cannot be found */
            NotFound = 0x0001,

            /**
             * The plugin is build with different version of PluginManager and
             * cannot be loaded.
             */
            WrongPluginVersion = 0x0002,

            /**
             * The plugin uses different interface than the interface
             * used by PluginManager and cannot be loaded.
             */
            WrongInterfaceVersion = 0x0004,

            /**
             * The plugin doesn't have metadata file or metadata file contains
             * errors.
             */
            WrongMetadataFile = 0x0008,

            /**
             * The plugin depends on another plugin, which cannot be loaded
             * (e.g. not found, conflict, wrong version).
             */
            UnresolvedDependency = 0x0010,

            /** %Plugin failed to load */
            LoadFailed = 0x0020,

            /** %Plugin is successfully loaded */
            LoadOk = 0x0040,

            /**
             * %Plugin is not loaded. %Plugin can be unloaded only
             * if is dynamic and is not required by any other plugin.
             */
            NotLoaded = 0x0100,

            /** %Plugin failed to unload */
            UnloadFailed = 0x0200,

            /**
             * %Plugin cannot be unloaded because another plugin is depending on
             * it. Unload that plugin first and try again.
             */
            IsRequired = 0x0400,

            /** %Plugin is static (and cannot be unloaded) */
            IsStatic = 0x0800,

            /**
             * %Plugin has active instance and cannot be unloaded. Destroy all
             * instances and try again.
             */
            IsUsed = 0x1000
        };

        /** @brief %Plugin version */
        static const int version;

        /**
         * @brief Register static plugin
         * @param plugin            %Plugin (name defined with
         *      PLUGIN_REGISTER())
         * @param _version          %Plugin version (must be the same as
         *      AbstractPluginManager::version)
         * @param interface         %Plugin interface string
         * @param instancer         %Plugin instancer function
         *
         * Used internally by PLUGIN_IMPORT() macro. There is absolutely no
         * need to use this directly.
         */
        static void importStaticPlugin(const std::string& plugin, int _version, const std::string& interface, Instancer instancer);

        /**
         * @brief Constructor
         * @param pluginDirectory   Directory where plugins will be searched,
         *      with tailing slash. No recursive processing is done.
         *
         * First goes through list of static plugins and finds ones that use
         * the same interface as this PluginManager instance. Then gets list of
         * all dynamic plugins in given directory.
         * @note Dependencies of static plugins are skipped, as static plugins
         *      should have all dependencies present. Also, dynamic plugins
         *      with the same name as another static plugin are skipped.
         * @see PluginManager::nameList()
         */
        inline AbstractPluginManager(const std::string& pluginDirectory): _pluginDirectory(pluginDirectory) {
            reloadPluginDirectory();
        }

        /**
         * @brief Destructor
         *
         * Destroys all plugin instances and unload all plugins.
         */
        virtual ~AbstractPluginManager();

        /** @brief %Plugin directory */
        inline std::string pluginDirectory() const { return _pluginDirectory; }

        /**
         * @brief Set another plugin directory
         * @param directory     %Plugin directory
         *
         * @see reloadPluginDirectory()
         */
        inline void setPluginDirectory(const std::string& directory) {
            _pluginDirectory = directory;
            reloadPluginDirectory();
        }

        /**
         * @brief Reload plugin directory
         *
         * Keeps loaded plugins untouched, removes unloaded plugins which are
         * not existing anymore and adds newly found plugins.
         *
         * @see reload()
         */
        virtual void reloadPluginDirectory();

        /** @brief List of all available plugin names */
        std::vector<std::string> pluginList() const;

        /**
         * @brief %Plugin metadata
         * @param plugin            %Plugin
         * @return Pointer to plugin metadata
         */
        const PluginMetadata* metadata(const std::string& plugin) const;

        /**
         * @brief Load state of a plugin
         * @param plugin            %Plugin
         * @return Load state of a plugin
         *
         * Static plugins always have AbstractPluginManager::IsStatic state.
         */
        LoadState loadState(const std::string& plugin) const;

        /**
         * @brief Load a plugin
         * @param _plugin           %Plugin
         * @return AbstractPluginManager::LoadOk on success,
         *      AbstractPluginManager::NotFound,
         *      AbstractPluginManager::WrongPluginVersion,
         *      AbstractPluginManager::WrongInterfaceVersion,
         *      AbstractPluginManager::UnresolvedDependency or
         *      AbstractPluginManager::LoadFailed  on failure.
         *
         * Checks whether a plugin is loaded, if not and loading is possible,
         * tries to load it. If the plugin has any dependencies, they are
         * recursively processed before loading given plugin.
         *
         * Calls reloadPluginMetadata().
         */
        virtual LoadState load(const std::string& _plugin);

        /**
         * @brief Unload a plugin
         * @param _plugin           %Plugin
         * @return AbstractPluginManager::NotLoaded on success,
         *      AbstractPluginManager::UnloadFailed,
         *      AbstractPluginManager::IsRequired or
         *      AbstractPluginManager::IsStatic on failure.
         *
         * Checks whether a plugin is loaded, if yes, tries to unload it and if
         * unload is successful, reloads its metadata. If the plugin is not
         * loaded, reloads its metadata and then returns its current load
         * state.
         *
         * Calls reloadPluginMetadata().
         */
        virtual LoadState unload(const std::string& _plugin);

        /**
         * @brief Reload a plugin
         * @return NotLoaded if the plugin was not loaded before, see load() and
         *      unload() for other values.
         *
         * If the plugin is loaded, unloads it, reloads its metadata and then
         * loads it again. If the plugin is unloaded, only reloads its metadata.
         *
         * Calls reloadPluginMetadata().
         */
        LoadState reload(const std::string& plugin);

    #ifdef DOXYGEN_GENERATING_OUTPUT
    private:
    #else
    protected:
    #endif

        /** @brief %Plugin object */
        struct PluginObject {
            LoadState loadState;                /**< @brief Load state */

            /**
             * @brief %Plugin interface
             *
             * Non-static plugins have this field empty.
             */
            std::string interface;

            /**
             * @brief %Plugin configuration
             *
             * Associated configuration file.
             */
            const Utility::Configuration configuration;

            PluginMetadata metadata;            /**< @brief %Plugin metadata */

            /**
             * @brief Associated plugin manager
             *
             * If set to zero, the plugin has not any associated plugin manager
             * and cannot be loaded.
             */
            AbstractPluginManager* manager;

            /** @brief %Plugin instancer function */
            Instancer instancer;

            /**
             * @brief %Plugin module handler
             *
             * Only for dynamic plugins
             */
            #ifndef _WIN32
            void* module;
            #else
            HMODULE module;
            #endif

            /**
             * @brief Constructor (dynamic plugins)
             * @param _metadata     %Plugin metadata filename
             * @param _manager      Associated plugin manager
             */
            inline PluginObject(const std::string& _metadata, AbstractPluginManager* _manager):
                configuration(_metadata, Utility::Configuration::Flag::ReadOnly), metadata(configuration), manager(_manager), instancer(nullptr), module(nullptr) {
                    if(configuration.isValid()) loadState = NotLoaded;
                    else loadState = WrongMetadataFile;
                }

            /**
             * @brief Constructor (static plugins)
             * @param _metadata     %Plugin metadata istream
             * @param _interface    Interface string
             * @param _instancer    Instancer function
             */
            inline PluginObject(std::istream& _metadata, std::string _interface, Instancer _instancer):
                loadState(IsStatic), interface(_interface), configuration(_metadata, Utility::Configuration::Flag::ReadOnly), metadata(configuration), manager(nullptr), instancer(_instancer), module(nullptr) {}
        };

        /** @brief Directory where to search for dynamic plugins */
        std::string _pluginDirectory;

        /** @brief %Plugin interface used by the plugin manager */
        virtual std::string pluginInterface() const = 0;

        /**
         * @brief Plugins
         *
         * Global storage of static, unloaded and loaded plugins.
         *
         * @note Development note: The map is accessible via function, not
         * directly, because we need to fill it with data from staticPlugins()
         * before first use.
         */
        static std::map<std::string, PluginObject*>* plugins();

        /**
         * @brief Reload plugin metadata
         * @param it        Iterator pointing to plugin object
         * @return False if plugin is not loaded and binary cannot be found,
         *      true otherwise.
         *
         * If the plugin is unloaded and belongs to current manager, checks
         * whether the plugin exists and reloads its metadata.
         *
         * Called from load(), unload() and reload().
         *
         * @bug Causes problems when calling this function in a cycle through
         * all plugins, as it can remove the plugin from vector and thus break
         * iterators.
         */
        virtual bool reloadPluginMetadata(std::map<std::string, PluginObject*>::iterator it);

        /**
         * @brief Add plugin to usedBy list
         * @param plugin    %Plugin which is used
         * @param usedBy    Which plugin uses it
         *
         * Because the plugin manager must be noticed about adding the plugin
         * to "used by" list, it must be done through this function.
         */
        virtual void addUsedBy(const std::string& plugin, const std::string& usedBy);

        /**
         * @brief Remove plugin from usedBy list
         * @param plugin    %Plugin which was used
         * @param usedBy    Which plugin used it
         *
         * Because the plugin manager must be noticed about removing the plugin
         * from "used by" list, it must be done through this function.
         */
        virtual void removeUsedBy(const std::string& plugin, const std::string& usedBy);

    private:
        /**
         * @brief Static plugin object
         *
         * See staticPlugins() for more information.
         */
        struct PLUGINMANAGER_LOCAL StaticPluginObject {
            std::string plugin;      /**< @brief %Plugin name */
            std::string interface;   /**< @brief %Plugin interface */

            /** @brief %Plugin instancer function */
            Instancer instancer;
        };

        /**
         * @brief Static plugins
         *
         * Temporary storage of all information needed to import static
         * plugins. They are imported to plugins() map on first call to
         * plugins(), because at that time it is safe to assume that all
         * static resources (plugin configuration files) are already
         * registered. After that, the storage is deleted and set to nullptr
         * to indicate that static plugins have been already processed.
         *
         * @note Development note: The vector is accessible via function, not
         * directly, because we don't know initialization order of static
         * members and thus the vector could be uninitalized when accessed
         * from PLUGIN_REGISTER().
         */
        PLUGINMANAGER_LOCAL static std::vector<StaticPluginObject>*& staticPlugins();

        std::map<std::string, std::vector<Plugin*> > instances;

        PLUGINMANAGER_LOCAL void registerInstance(const std::string& plugin, Plugin* instance, const Utility::Configuration** configuration, const PluginMetadata** metadata);
        PLUGINMANAGER_LOCAL void unregisterInstance(const std::string& plugin, Plugin* instance);
};

/**
@brief Import static plugin
@param name      Static plugin name (defined with PLUGIN_REGISTER())
@hideinitializer

If static plugins are compiled into dynamic library or directly into the
executable, they should be automatically loaded at startup thanks to
AUTOMATIC_INITALIZER() and AUTOMATIC_FINALIZER() macros.

If static plugins are compiled into static library, they are not
automatically loaded at startup, so you need to load them explicitly by
calling PLUGIN_IMPORT() at the beggining of main() function. You can also
wrap these macro calls into another function (which will then be compiled
into dynamic library or main executable) and use AUTOMATIC_INITIALIZER()
macro for automatic call.
@attention This macro should be called outside of any namespace. If you are
running into linker errors with `pluginInitializer_*`, this could be the
problem. See RESOURCE_INITIALIZE() documentation for more information.
 */
#define PLUGIN_IMPORT(name)                                                   \
    extern int pluginInitializer_##name();                                    \
    pluginInitializer_##name();                                               \
    RESOURCE_INITIALIZE(name)

} namespace Utility {

/** @debugoperator{Corrade::PluginManager::AbstractPluginManager} */
Debug PLUGINMANAGER_EXPORT operator<<(Debug debug, PluginManager::AbstractPluginManager::LoadState value);

}}

#endif
