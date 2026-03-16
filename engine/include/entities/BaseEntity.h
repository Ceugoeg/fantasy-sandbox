#pragma once

// 只是告诉编译器“有一个叫 World 的类存在”，而不去 #include "core/World.h"。
// 因为 World.h 里面又包含了 BaseEntity.h 的引用，如果互相 include 会导致严重的“循环依赖”编译报错。
// 只要我们在头文件中只使用 World 的引用（&）或指针（*），前向声明就足够了。
namespace fsb {
namespace core {
    class World;
}
}

namespace fsb {
namespace entities {

class BaseEntity {
protected:
    // 将基础属性设为 protected，这样未来具体的模板实体类（派生类）可以直接访问它们
    int id_;
    int x_;
    int y_;

public:
    BaseEntity(int id, int x, int y) : id_(id), x_(x), y_(y) {}

    virtual ~BaseEntity() = default;

    // 强制所有继承自 BaseEntity 的类必须实现自己的更新逻辑。
    // 传入 World 的引用，让实体能够“感知”周围的环境。
    virtual void update(core::World& world) = 0;

    // --- 基础 Getter 与 Setter ---
    // 这些非虚函数在编译期就能确定地址，调用效率极高
    int getId() const { return id_; }
    int getX() const { return x_; }
    int getY() const { return y_; }
    
    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
    }
};

} // namespace entities
} // namespace fsb
