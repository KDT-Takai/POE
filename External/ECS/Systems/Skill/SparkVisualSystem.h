#pragma once
#include "../../Registry/Registry.h"
#include "../../Components/Physics/Transform/Transform.h"
#include "../../Components/PlayerSkill/SparkVisual.h"

class SparkVisualSystem {
public:
    void Update(Registry& registry) {
        auto view = registry.View<TransformComponent, SparkVisualComponent>();

        for (auto entity : view) {
            auto& trans = registry.GetComponent<TransformComponent>(entity);
            auto& sparkVis = registry.GetComponent<SparkVisualComponent>(entity);

            if (sparkVis.style == VisualStyle::Explosion) continue;

            sparkVis.trailHistory.push_back(trans.position);

            if (sparkVis.trailHistory.size() > sparkVis.maxTrailLength) {
                sparkVis.trailHistory.erase(sparkVis.trailHistory.begin());
            }
        }
    }
};