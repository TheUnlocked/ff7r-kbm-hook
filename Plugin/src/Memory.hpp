namespace Memory {
    const std::uintptr_t base_addr = dku::Hook::Module::get().base();

    template<typename T>
    [[nodiscard]]
    inline T& deref(std::uintptr_t base, std::ptrdiff_t offset) {
        return *reinterpret_cast<T*>(base + offset);
    }

    template<typename T>
    [[nodiscard]]
    inline T& deref_static(std::ptrdiff_t offset) {
        return deref<T>(base_addr, offset);
    }

    [[nodiscard]]
    inline uintptr_t get_code_address(std::ptrdiff_t offset) {
        return base_addr + offset;
    }
}