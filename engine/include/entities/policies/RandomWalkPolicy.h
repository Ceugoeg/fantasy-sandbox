#pragma once

#include <cstdlib> // 提供 std::rand()

// 前向声明，解决类型引用问题
namespace fsb {
namespace core {
    class World;
}
namespace entities {
    class BaseEntity;
}
}

namespace fsb {
namespace entities {
namespace policies {

class RandomWalkPolicy {
public:
    // 策略的核心执行接口。
    // Entity 模板在调用时，会将自身 (*this) 和世界 (world) 传递进来。
    void execute(BaseEntity& entity, core::World& world) {
        // 1. 产生随机位移：让 x 和 y 在 -1, 0, 1 之间随机变动
        int dx = (std::rand() % 3) - 1;
        int dy = (std::rand() % 3) - 1;

        // 计算预期的新坐标
        int new_x = entity.getX() + dx;
        int new_y = entity.getY() + dy;

        // 2. 世界规则校验与交互
        // 在移动前，必须询问世界：这个坐标是否越界？地形是否允许通行？
        // 假设 getTerrainAt 返回 0 代表平地（可通行），-1 代表越界
        if (world.getTerrainAt(new_x, new_y) == 0) {
            // 校验通过，更新实体的物理坐标
            entity.setPosition(new_x, new_y);
        }
    }
};

} // namespace policies
} // namespace entities
} // namespace fsb
