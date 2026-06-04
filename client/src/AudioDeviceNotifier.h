#pragma once
#include <mmdeviceapi.h>
#include <functional>

class AudioDeviceNotifier : public IMMNotificationClient {
public:
    static AudioDeviceNotifier& getInstance() {
        static AudioDeviceNotifier instance;
        return instance;
    }

    void setCallback(std::function<void()> callback) {
        m_callback = callback;
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == __uuidof(IMMNotificationClient)) {
            *ppvObject = this;
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    // IMMNotificationClient
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR) override {
        if (flow == eRender && role == eConsole && m_callback) {
            m_callback();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR, DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

private:
    AudioDeviceNotifier() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&m_enumerator);
        if (m_enumerator) {
            m_enumerator->RegisterEndpointNotificationCallback(this);
        }
    }

    ~AudioDeviceNotifier() {
        if (m_enumerator) {
            m_enumerator->UnregisterEndpointNotificationCallback(this);
            m_enumerator->Release();
        }
        CoUninitialize();
    }

    IMMDeviceEnumerator* m_enumerator = nullptr;
    std::function<void()> m_callback;
};