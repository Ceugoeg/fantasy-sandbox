#include "core/World.h"
// 引入即将编写的基类头文件，因为我们需要调用实体的接口
#include "entities/BaseEntity.h" 

#include <algorithm>
#include <sstream>

namespace fsb {
namespace core {

World::World(int width, int height)
    : width_(width), height_(height), current_tick_(0) {
    // 创世第一步：初始化地形网格。
    // std::vector 的 resize 会分配内存并用指定的值（这里是 0，代表平地）填满。
    terrain_.resize(width_ * height_, 0);
}

void World::tick() {
    current_tick_++;
    
    // 遍历所有实体，调用它们的更新逻辑。
    // 将 *this（世界本身的引用）传给实体，让实体能够“观察”世界（比如获取周围地形）。
    for (auto& entity : entities_) {
        entity->update(*this);
    }
}

void World::addEntity(std::shared_ptr<fsb::entities::BaseEntity> entity) {
    entities_.push_back(entity);
}

void World::removeEntity(int entity_id) {
    // 【C++ 经典惯用法：Erase-Remove Idiom】
    // std::remove_if 并不会真正删除元素，而是把需要保留的元素往前挪，返回一个逻辑上的新结尾迭代器。
    // 然后再调用 vector 的 erase 方法，把尾部多余的废弃元素彻底抹除。
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [entity_id](const std::shared_ptr<fsb::entities::BaseEntity>& e) {
                return e->getId() == entity_id;
            }),
        entities_.end()
    );
}

int World::getTerrainAt(int x, int y) const {
    // 越界检查：如果越界，返回 -1（可以被定义为虚空或阻挡物）
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return -1; 
    }
    // 【一维模拟二维原理】：y 乘以宽度，再加上 x，精准命中内存中的一维地址。
    return terrain_[y * width_ + x];
}

std::vector<std::shared_ptr<fsb::entities::BaseEntity>> World::getEntitiesInRange(int x, int y, int radius) const {
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> result;
    
    for (const auto& entity : entities_) {
        int ex = entity->getX();
        int ey = entity->getY();
        
        // 性能优化细节：计算欧几里得距离时，比较距离平方而不是实际距离。
        // 即判断 $d^2 \le r^2$ 而不是 $d \le r$，从而避免极其消耗 CPU 指令周期的 std::sqrt 开平方运算。
        int dist_sq = (ex - x) * (ex - x) + (ey - y) * (ey - y);
        if (dist_sq <= radius * radius) {
            result.push_back(entity);
        }
    }
    return result;
}

std::string World::exportStateAsJson() const {
    // 手工拼接 JSON 字符串，准备通过网络发给 Node.js
    std::stringstream ss;
    ss << "{";
    ss << "\"tick\": " << current_tick_ << ", ";
    ss << "\"entities\": [";
    
    for (size_t i = 0; i < entities_.size(); ++i) {
        ss << "{";
        ss << "\"id\": " << entities_[i]->getId() << ", ";
        ss << "\"x\": " << entities_[i]->getX() << ", ";
        ss << "\"y\": " << entities_[i]->getY() << ", ";
        ss << "\"state\": \"" << entities_[i]->getStateAsString() << "\"";
        ss << "}";
        
        // 如果不是最后一个实体，加上逗号分隔
        if (i < entities_.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    ss << "}";
    
    return ss.str();
}

} // namespace core
} // namespace fsb
