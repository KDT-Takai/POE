#pragma once

enum class CurrencyType { Transmutation, Regal, Chaos };

struct CurrencyPickupComponent {
    CurrencyType type = CurrencyType::Transmutation;
};
