#pragma once

#include <memory>
#include <QString>

class QLockFile;


class InstanceLock
{
public:
    static constexpr int kMaxSlots = 32;

    InstanceLock();
    ~InstanceLock();

    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;

    void acquire();

    int index() const { return index_; }
    bool isPrimary() const { return index_ == 1; }

private:
    static QString slotPath(int slot);

    std::unique_ptr<QLockFile> lock_;
    int index_ = 0;
};
