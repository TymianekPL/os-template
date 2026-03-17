#pragma once

#include <atomic>
#include "operations.h"

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
               NO_ASAN Iterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               NO_ASAN Iterator operator++(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               NO_ASAN Iterator& operator--() noexcept
               {
                    this->head = this->head->previous;
                    return *this;
               }
               NO_ASAN Iterator operator--(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->previous;
                    return copy;
               }
               NO_ASAN bool operator==(const Iterator& other) const noexcept { return other.head == this->head; }
               NO_ASAN bool operator!=(const Iterator& other) const noexcept { return other.head != this->head; }
               NO_ASAN TNode& operator*() noexcept { return this->head->data; }
               NO_ASAN const TNode& operator*() const noexcept { return this->head->data; }
               ListEntry<TNode>* head;
          };
          struct ConstIterator
          {
               NO_ASAN ConstIterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               NO_ASAN ConstIterator operator++(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               NO_ASAN ConstIterator& operator--() noexcept
               {
                    this->head = this->head->previous;
                    return *this;
               }
               NO_ASAN ConstIterator operator--(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->previous;
                    return copy;
               }
               NO_ASAN bool operator==(const ConstIterator& other) const noexcept { return other.head == this->head; }
               NO_ASAN bool operator!=(const ConstIterator& other) const noexcept { return other.head != this->head; }
               NO_ASAN const TNode& operator*() const noexcept { return this->head->data; }
               const ListEntry<TNode>* head;
          };

          [[nodiscard]] NO_ASAN Iterator begin() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{this->head};
          }
          [[nodiscard]] NO_ASAN Iterator end() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{nullptr};
          }

          [[nodiscard]] NO_ASAN ConstIterator begin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] NO_ASAN ConstIterator end() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          [[nodiscard]] NO_ASAN ConstIterator cbegin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] NO_ASAN ConstIterator cend() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          ListEntry<TNode>* head;
     };
     template <typename TNode> struct SingleList
     {
          struct Iterator
          {
               NO_ASAN Iterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               NO_ASAN Iterator operator++(int) noexcept
               {
                    Iterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               NO_ASAN bool operator==(const Iterator& other) const noexcept { return other.head == this->head; }
               NO_ASAN bool operator!=(const Iterator& other) const noexcept { return other.head != this->head; }
               NO_ASAN TNode& operator*() noexcept { return this->head->data; }
               NO_ASAN const TNode& operator*() const noexcept { return this->head->data; }
               SingleListEntry<TNode>* head;
          };
          struct ConstIterator
          {
               NO_ASAN ConstIterator& operator++() noexcept
               {
                    this->head = this->head->next;
                    return *this;
               }
               NO_ASAN ConstIterator operator++(int) noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
               NO_ASAN bool operator==(const ConstIterator& other) const noexcept { return other.head == this->head; }
               NO_ASAN bool operator!=(const ConstIterator& other) const noexcept { return other.head != this->head; }
               NO_ASAN const TNode& operator*() const noexcept { return this->head->data; }
               mutable const SingleListEntry<TNode>* head;
          };

          [[nodiscard]] NO_ASAN Iterator begin() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{this->head};
          }
          [[nodiscard]] NO_ASAN Iterator end() noexcept // NOLINT(readability-identifier-naming)
          {
               return Iterator{nullptr};
          }

          [[nodiscard]] NO_ASAN ConstIterator begin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] NO_ASAN ConstIterator end() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          [[nodiscard]] NO_ASAN ConstIterator cbegin() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{this->head};
          }
          [[nodiscard]] NO_ASAN ConstIterator cend() const noexcept // NOLINT(readability-identifier-naming)
          {
               return ConstIterator{nullptr};
          }

          SingleListEntry<TNode>* head;
     };

     template <typename TNode> struct AtomicSingleList
     {
          std::atomic<SingleListEntry<TNode>*> head{nullptr};

          NO_ASAN void Push(SingleListEntry<TNode>* node) noexcept
          {
               SingleListEntry<TNode>* oldHead = nullptr;
               do
               {
                    oldHead = head.load(std::memory_order::acquire);
                    node->next = oldHead;
               } while (
                   !head.compare_exchange_weak(oldHead, node, std::memory_order::release, std::memory_order::acquire));
          }

          NO_ASAN SingleListEntry<TNode>* Pop() noexcept
          {
               SingleListEntry<TNode>* oldHead = nullptr;
               SingleListEntry<TNode>* newHead = nullptr;
               do
               {
                    oldHead = head.load(std::memory_order::acquire);
                    if (oldHead == nullptr) return nullptr;
                    newHead = oldHead->next;
               } while (!head.compare_exchange_weak(oldHead, newHead, std::memory_order::release,
                                                    std::memory_order::acquire));
               return oldHead;
          }

          [[nodiscard]] NO_ASAN bool IsEmpty() const noexcept
          {
               return head.load(std::memory_order::acquire) == nullptr;
          }
     };
} // namespace structures
