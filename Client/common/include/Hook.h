#ifndef COMMON_HOOK_H
#define COMMON_HOOK_H

#include <Windows.h>
#include <cstdint>
#include "detours.h" // Asegúrate de tener detours.lib en tu proyecto
#include "Types.h"
#include "Exceptions.h"

namespace Hooks {

    enum class CallConvention
    {
        stdcall_t,
        cdecl_t,
        fastcall_t // Añadido fastcall por si interceptas funciones internas del engine
    };

    // Helper para definir la convención de llamada
    template <CallConvention cc, typename retn, typename ...args>
    struct convention;

    template <typename retn, typename ...args>
    struct convention<CallConvention::stdcall_t, retn, args...>
    {
        typedef retn (__stdcall *type)(args ...);
    };

    template <typename retn, typename ...args>
    struct convention<CallConvention::cdecl_t, retn, args...>
    {
        typedef retn (__cdecl *type)(args ...);
    };

    // Fallout 4 usa mucho fastcall (__rcall en x64)
    template <typename retn, typename ...args>
    struct convention<CallConvention::fastcall_t, retn, args...>
    {
        typedef retn (__fastcall *type)(args ...);
    };

    template <CallConvention cc, typename retn, typename ...args>
    class Hook
    {
        typedef typename convention<cc, retn, args...>::type type;

        uintptr_t orig_; // Usamos uintptr_t para compatibilidad x64
        type detour_;

        bool is_applied_;
        bool has_open_transaction_;

        void transaction_begin()
        {
            if (DetourTransactionBegin() != NO_ERROR)
                throw Exceptions::Core::Exceptions::DetourException("A pending transaction already exists");
            has_open_transaction_ = true;
        }

        void transaction_commit()
        {
            if (DetourTransactionCommit() != NO_ERROR)
                throw Exceptions::Core::Exceptions::DetourException("Error committing detour transaction");
            has_open_transaction_ = false;
        }

        static void update_thread(HANDLE hThread)
        {
            DetourUpdateThread(hThread);
        }

    public:
        Hook() : orig_(0), detour_(0), is_applied_(false), has_open_transaction_(false) {}

        ~Hook()
        {
            if (has_open_transaction_) DetourTransactionAbort();
            remove();
        }

        // pFunc ahora acepta uintptr_t para que funcione con GameAddr::GetUIntPtr()
        void apply(uintptr_t pFunc, type detour)
        {
            if (is_applied_) return;

            detour_ = detour;
            orig_ = pFunc;

            transaction_begin();
            update_thread(GetCurrentThread());
            
            // Detours requiere un puntero al puntero de la función original
            DetourAttach(reinterpret_cast<void **>(&orig_), reinterpret_cast<void *>(detour_));
            
            transaction_commit();
            is_applied_ = true;
        }

        void remove()
        {
            if (!is_applied_) return;

            transaction_begin();
            update_thread(GetCurrentThread());
            DetourDetach(reinterpret_cast<void **>(&orig_), reinterpret_cast<void *>(detour_));
            transaction_commit();

            is_applied_ = false;
        }

        // Esta es la función que llamas desde tu lambda en main.cpp
        retn call_orig(args ... p)
        {
            return reinterpret_cast<type>(orig_)(p...);
        }

        uintptr_t get_orig() const { return orig_; }
        bool is_applied() const { return is_applied_; }
    };
}

#endif