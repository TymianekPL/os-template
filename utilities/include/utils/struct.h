#pragma once

#include <atomic>

namespace structures
{
     template <typename TNode> struct ListEntry
     {
          ListEntry* next{};
          ListEntry* previous{};
          TNode data{};
     };
     template <typename TNode> struct SingleListEntry
     {
          SingleListEntry* next{};
          TNode data{};
     };

     template <typename TNode> struct LinkedList
     {
          struct Iterator
          {
               Iterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               Iterator operator++(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               Iterator& operator--() noexcept
               {
                    this->head = this->head->previous;
                    return *this;
               }
               Iterator operator--(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->previous;
                    return copy;
               }
               bool operator==(const Iterator& other) const noexcept { return other.head == this->head; }
               bool operator!=(const Iterator& other) const noexcept { return other.head != this->head; }
               TNode& operator*() noexcept { return this->head->data; }
               const TNode& operator*() const noexcept { return this->head->data; }
               ListEntry<TNode>* head;
          };
          struct ConstIterator
          {
               ConstIterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               ConstIterator operator++(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               ConstIterator& operator--() noexcept
               {
                    this->head = this->head->previous;
                    return *this;
               }
               ConstIterator operator--(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->previous;
                    return copy;
               }
               bool operator==(const ConstIterator& other) const noexcept { return other.head == this->head; }
               bool operator!=(const ConstIterator& other) const noexcept { return other.head != this->head; }
               const TNode& operator*() const noexcept { return this->head->data; }
               const ListEntry<TNode>* head;
          };

          [[nodiscard]] Iterator begin() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{this->head};
          }
          [[nodiscard]] Iterator end() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{nullptr};
          }

          [[nodiscard]] ConstIterator begin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] ConstIterator end() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          [[nodiscard]] ConstIterator cbegin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] ConstIterator cend() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          ListEntry<TNode>* head;
     };
     template <typename TNode> struct SingleList
     {
          struct Iterator
          {
               Iterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               Iterator operator++(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               bool operator==(const Iterator& other) const noexcept { return other.head == this->head; }
               bool operator!=(const Iterator& other) const noexcept { return other.head != this->head; }
               TNode& operator*() noexcept { return this->head->data; }
               const TNode& operator*() const noexcept { return this->head->data; }
               SingleListEntry<TNode>* head;
          };
          struct ConstIterator
          {
               ConstIterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               ConstIterator operator++(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               bool operator==(const ConstIterator& other) const noexcept { return other.head == this->head; }
               bool operator!=(const ConstIterator& other) const noexcept { return other.head != this->head; }
               const TNode& operator*() const noexcept { return this->head->data; }
               mutable const SingleListEntry<TNode>* head;
          };

          [[nodiscard]] Iterator begin() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{this->head};
          }
          [[nodiscard]] Iterator end() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{nullptr};
          }

          [[nodiscard]] ConstIterator begin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] ConstIterator end() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          [[nodiscard]] ConstIterator cbegin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] ConstIterator cend() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          SingleListEntry<TNode>* head;
     };

     template <typename TNode> struct AtomicSingleList
     {
          std::atomic<SingleListEntry<TNode>*> head{nullptr};

          void Push(SingleListEntry<TNode>* node) noexcept
          {
               SingleListEntry<TNode>* oldHead = nullptr;
               do
               {
                    oldHead = head.load(std::memory_order_acquire);
                    node->next = oldHead;
               } while (
                   !head.compare_exchange_weak(oldHead, node, std::memory_order::release, std::memory_order_acquire));
          }

          SingleListEntry<TNode>* Pop() noexcept
          {
               SingleListEntry<TNode>* oldHead = nullptr;
               SingleListEntry<TNode>* newHead = nullptr;
               do
               {
                    oldHead = head.load(std::memory_order::acquire);
                    if (oldHead == nullptr) return nullptr;
                    newHead = oldHead->next;
               } while (!head.compare_exchange_weak(oldHead, newHead, std::memory_order::release,
                                                    std::memory_order_acquire));
               return oldHead;
          }

          [[nodiscard]] bool IsEmpty() const noexcept { return head.load(std::memory_order_acquire) == nullptr; }
     };
} // namespace structures
