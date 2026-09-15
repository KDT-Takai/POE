#pragma once

enum class CurrencyType { Transmutation, Regal, Chaos, Regret, Alchemy, Augmentation, Annulment, Chance, Scouring, Count };

struct CurrencyPickupComponent {
    CurrencyType type = CurrencyType::Transmutation;
};
