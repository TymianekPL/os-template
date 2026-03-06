#pragma once

namespace structures
{
     template <typename TNode> struct ListEntry
     {
          ListEntry* next{};
          ListEntry* previous{};
          TNode data{};
     };

     template <typename TNode> struct LinkedList
     {
          struct Iterator
          {
               Iterator operator++()
               {
                    this->head = this->head->next;
                    return *this;
               }
               Iterator operator++(int)
               {
                    Iterator copy = *this;
                    this->head = this->head->next;
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
               ConstIterator operator++() const noexcept { this->head = this->head->next; }
               ConstIterator operator++(int) const noexcept
               {
                    ConstIterator copy = *this;
                    this->head = this->head->next;
                    return copy;
               }
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

          ListEntry<TNode>* head;
     };
} // namespace structures
