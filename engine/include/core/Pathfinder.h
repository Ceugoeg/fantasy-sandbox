#pragma once

#include <vector>
#include <utility>

namespace fsb {
namespace core {

class World; // 前向声明，解耦头文件依赖

class Pathfinder {
public:
    // A* 启发式寻路引擎
    // 计算从 (start_x, start_y) 到 (target_x, target_y) 的最短避障路径
    // 返回结果不包含起点，按移动顺序（从下一步到终点）排列。若无路可通则返回空 vector
    static std::vector<std::pair<int, int>> findPath(
        const World& world,
        int start_x, int start_y,
        int target_x, int target_y
    );

    // BFS 邻域扫描雷达
    // 寻找离起点最近的、且地貌 ID 落在 [biome_min, biome_max] 范围内的安全坐标
    // max_radius 限制最大探测层数，防止在绝境中耗尽 CPU 算力
    // 若在半径内未找到，则返回 {-1, -1}
    static std::pair<int, int> findNearestBiome(
        const World& world,
        int start_x, int start_y,
        int biome_min, int biome_max,
        int max_radius = 15
    );
};

} // namespace core
} // namespace fsb