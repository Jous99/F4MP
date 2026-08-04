// Soporte para cargar F4MP como plugin de F4SE (Fallout 4 Script Extender).
//
// F4SE carga automaticamente los DLL que esten en  Data\F4SE\Plugins\  y que
// exporten estos simbolos. El arranque real de F4MP ocurre en DllMain (main.cpp),
// asi que aqui solo declaramos la informacion que F4SE necesita para aceptar y
// mantener cargado el plugin.
//
// Se declara la estructura de F4SE de forma minima (sin necesitar el SDK completo).

#include <cstdint>

// ---------------------------------------------------------------------------
//  API nueva (F4SE de Next-Gen / Anniversary): estructura de version exportada
// ---------------------------------------------------------------------------
struct F4SEPluginVersionData
{
    enum { kVersion = 1 };

    enum {
        kVersionIndependent_AddressLibraryPostNG = 1 << 0,
        kVersionIndependent_Signatures           = 1 << 1,
        kVersionIndependent_StructsPost1105      = 1 << 2,
    };

    enum {
        kVersionIndependentEx_NoStructUse = 1 << 0,
    };

    uint32_t dataVersion;            // = kVersion
    uint32_t pluginVersion;          // version de nuestro plugin
    char     name[256];              // nombre ASCII terminado en null
    char     author[256];            // autor (puede ir vacio)
    char     supportEmail[256];      // email de soporte (puede ir vacio)
    uint32_t versionIndependenceEx;  // flags
    uint32_t versionIndependence;    // flags
    uint32_t compatibleVersions[16]; // lista terminada en 0; 0 = todas si es independiente de version
    uint32_t seVersionRequired;      // version minima de F4SE, 0 = cualquiera
};

extern "C" __declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
{
    F4SEPluginVersionData::kVersion,
    1,                    // pluginVersion
    "F4MP",               // name
    "F4MP",               // author
    "",                   // supportEmail
    // F4MP usa pattern scanning y no depende del layout de estructuras del juego,
    // asi que es independiente de la version del juego.
    F4SEPluginVersionData::kVersionIndependentEx_NoStructUse,
    F4SEPluginVersionData::kVersionIndependent_Signatures,
    { 0 },                // compatibleVersions: todas
    0                     // seVersionRequired: cualquiera
};

// ---------------------------------------------------------------------------
//  API clasica (F4SE antiguo): por compatibilidad hacia atras.
//  Si F4SE encuentra F4SEPlugin_Version, ignora estas dos.
// ---------------------------------------------------------------------------
struct PluginInfo
{
    enum { kInfoVersion = 1 };
    uint32_t     infoVersion;
    const char*  name;
    uint32_t     version;
};

extern "C" __declspec(dllexport) bool F4SEPlugin_Query(const void* /*f4se*/, PluginInfo* info)
{
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name = "F4MP";
    info->version = 1;
    return true;
}

extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const void* /*f4se*/)
{
    // El arranque real de F4MP se dispara desde DllMain (main.cpp) al cargar el DLL.
    // Aqui no hace falta nada: solo confirmar a F4SE que la carga fue correcta.
    return true;
}
