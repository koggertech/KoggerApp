#pragma once

#include <memory>
#include <QString>

class QLockFile;


class InstanceLock
{
public:
    static constexpr int kMaxInstances = 2;

    InstanceLock();
    ~InstanceLock();

    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;

    bool acquire();
    int index() const { return index_; }
    bool isPrimary() const { return index_ == 1; }

private:
    static QString slotPath(int slot);

    std::unique_ptr<QLockFile> lock_;
    int index_ = 0;
};
