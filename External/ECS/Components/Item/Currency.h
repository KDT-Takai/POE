#pragma once

enum class CurrencyType { Transmutation, Regal, Chaos, Regret };

struct CurrencyPickupComponent {
    CurrencyType type = CurrencyType::Transmutation;
};
