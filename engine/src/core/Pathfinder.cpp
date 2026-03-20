#include "core/Pathfinder.h"
#include "core/World.h"

#include <queue>
#include <cmath>
#include <algorithm>
#include <climits>

namespace fsb {
namespace core {

// 四向移动向量：上、下、左、右
static const int DX[] = {0, 0, -1, 1};
static const int DY[] = {-1, 1, 0, 0};

// A* 优先队列节点结构
struct AStarNode {
    int x, y;
    int f_score; // F = G + H

    // std::priority_queue 默认是大顶堆，我们需要小顶堆（F值最小的优先）
    bool operator>(const AStarNode& other) const {
        return f_score > other.f_score;
    }
};

std::vector<std::pair<int, int>> Pathfinder::findPath(
    const World& world, 
    int start_x, int start_y, 
    int target_x, int target_y) 
{
    std::vector<std::pair<int, int>> path;

    // 边界与绝境预检
    if (world.isBlocked(target_x, target_y) || 
        (start_x == target_x && start_y == target_y)) {
        return path; 
    }

    int width = world.getWidth();
    int height = world.getHeight();
    int map_size = width * height;

    // 极客内存优化：直接分配与地图大小相等的一维数组，放弃极度拖慢性能的 unordered_map
    // came_from 存储每个节点的父节点一维索引
    std::vector<int> came_from(map_size, -1);
    // cost_so_far 存储从起点到当前节点的最短步数 (G值)
    std::vector<int> cost_so_far(map_size, INT_MAX);

    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> frontier;

    int start_idx = start_y * width + start_x;
    frontier.push({start_x, start_y, 0});
    cost_so_far[start_idx] = 0;

    bool found = false;

    while (!frontier.empty()) {
        auto current = frontier.top();
        frontier.pop();

        if (current.x == target_x && current.y == target_y) {
            found = true;
            break;
        }

        int current_idx = current.y * width + current.x;

        // 探索四周
        for (int i = 0; i < 4; ++i) {
            int next_x = current.x + DX[i];
            int next_y = current.y + DY[i];

            // 越界或物理阻挡拦截
            if (next_x < 0 || next_x >= width || next_y < 0 || next_y >= height) continue;
            if (world.isBlocked(next_x, next_y)) continue;

            int next_idx = next_y * width + next_x;
            // 网格图中每走一步代价为 1
            int new_cost = cost_so_far[current_idx] + 1;

            if (new_cost < cost_so_far[next_idx]) {
                cost_so_far[next_idx] = new_cost;
                // 曼哈顿距离启发函数 (H)
                int heuristic = std::abs(target_x - next_x) + std::abs(target_y - next_y);
                int priority = new_cost + heuristic;
                
                frontier.push({next_x, next_y, priority});
                came_from[next_idx] = current_idx;
            }
        }
    }

    // 路径重构 (回溯法)
    if (found) {
        int current_idx = target_y * width + target_x;
        int start_index = start_y * width + start_x;

        while (current_idx != start_index) {
            int cx = current_idx % width;
            int cy = current_idx / width;
            path.push_back({cx, cy});
            
            current_idx = came_from[current_idx];
            if (current_idx == -1) break; // 理论上不可能发生，防爆盾
        }

        // 因为是从终点往起点追溯的，所以需要翻转数组
        std::reverse(path.begin(), path.end());
    }

    return path;
}

std::pair<int, int> Pathfinder::findNearestBiome(
    const World& world, 
    int start_x, int start_y, 
    int biome_min, int biome_max, 
    int max_radius) 
{
    int width = world.getWidth();
    int height = world.getHeight();
    int map_size = width * height;

    // 存储 {当前坐标, 当前扩散层数}
    std::queue<std::pair<std::pair<int, int>, int>> frontier;
    // 同样使用一维 bool 数组做访问标记，避免重复绕圈
    std::vector<bool> visited(map_size, false);

    frontier.push({{start_x, start_y}, 0});
    visited[start_y * width + start_x] = true;

    while (!frontier.empty()) {
        auto current = frontier.front();
        frontier.pop();

        int cx = current.first.first;
        int cy = current.first.second;
        int current_radius = current.second;

        // 【剪枝核心】：如果已经超过最大探测半径，放弃挣扎，直接截断
        if (current_radius > max_radius) {
            break; 
        }

        // 抵达合法群落！立即返回该坐标
        int current_biome = world.getTerrainAt(cx, cy);
        if (current_biome >= biome_min && current_biome <= biome_max) {
            return {cx, cy};
        }

        // 像水波纹一样继续向外扩散四向
        for (int i = 0; i < 4; ++i) {
            int nx = cx + DX[i];
            int ny = cy + DY[i];

            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            
            int n_idx = ny * width + nx;
            // 必须是没走过的，且绝对不能穿透树木或石头
            if (!visited[n_idx] && !world.isBlocked(nx, ny)) {
                visited[n_idx] = true;
                frontier.push({{nx, ny}, current_radius + 1});
            }
        }
    }

    // 周围 [max_radius] 范围内全是异乡，找不到回家的路
    return {-1, -1};
}

} // namespace core
} // namespace fsb