#include "vallocator.h"
#include <algorithm>

namespace memory
{
     constexpr std::size_t PAGE_SIZE = 4096;

     struct FreeNode
     {
          FreeNode* next;
     };

     bool VADNodeAllocator::AllocateNodePage()
     {
          std::uintptr_t physPage = physicalAllocator.AllocatePage(PFNUse::KernelHeap);
          if (physPage == ~0uz) return false;

          auto* page = reinterpret_cast<struct FreeNode*>(physPage + physicalAllocator.physToVirtOffset);

          std::size_t nodesPerPage = PAGE_SIZE / sizeof(VADNode);
          auto* nodeArray = reinterpret_cast<VADNode*>(page);

          for (std::size_t i = 0; i < nodesPerPage; i++)
          {
               auto* freeNode = reinterpret_cast<struct FreeNode*>(&nodeArray[i]);
               freeNode->next = reinterpret_cast<struct FreeNode*>(_freeList);
               this->_freeList = freeNode;
          }

          return true;
     }

     VADNode* VADNodeAllocator::AllocateNode(const VADEntry& entry)
     {
          if (this->_freeList == nullptr)
          {
               if (!AllocateNodePage()) return nullptr;
          }

          struct FreeNode* freeNode = reinterpret_cast<struct FreeNode*>(_freeList);
          this->_freeList = freeNode->next;

          auto* node = reinterpret_cast<VADNode*>(freeNode);
          new (node) VADNode(entry);

          this->_nodesAllocated++;
          return node;
     }

     void VADNodeAllocator::FreeNode(VADNode* node)
     {
          if (!node) return;

          node->~VADNode();

          auto* freeNode = reinterpret_cast<struct FreeNode*>(node);
          freeNode->next = reinterpret_cast<struct FreeNode*>(_freeList);
          this->_freeList = freeNode;

          this->_nodesFreed++;
     }

     void VirtualMemoryAllocator::UpdateMaxEnd(VADNode* node)
     {
          if (!node) return;

          node->maxEnd = node->EndAddress();
          if (node->left && node->left->maxEnd > node->maxEnd) node->maxEnd = node->left->maxEnd;
          if (node->right && node->right->maxEnd > node->maxEnd) node->maxEnd = node->right->maxEnd;
     }

     void VirtualMemoryAllocator::UpdateMaxEndUpwards(VADNode* node)
     {
          while (node)
          {
               UpdateMaxEnd(node);
               node = node->parent;
          }
     }

     void VirtualMemoryAllocator::RotateLeft(VADNode* x)
     {
          VADNode* y = x->right;
          x->right = y->left;

          if (y->left) y->left->parent = x;

          y->parent = x->parent;

          if (!x->parent) this->_root = y;
          else if (x == x->parent->left)
               x->parent->left = y;
          else
               x->parent->right = y;

          y->left = x;
          x->parent = y;

          UpdateMaxEnd(x);
          UpdateMaxEnd(y);
     }

     void VirtualMemoryAllocator::RotateRight(VADNode* y)
     {
          VADNode* x = y->left;
          y->left = x->right;

          if (x->right) x->right->parent = y;

          x->parent = y->parent;

          if (!y->parent) this->_root = x;
          else if (y == y->parent->left)
               y->parent->left = x;
          else
               y->parent->right = x;

          x->right = y;
          y->parent = x;

          UpdateMaxEnd(y);
          UpdateMaxEnd(x);
     }

     void VirtualMemoryAllocator::FixInsert(VADNode* z)
     {
          while (z->parent && z->parent->colour == RBColour::Red)
          {
               if (z->parent == z->parent->parent->left)
               {
                    VADNode* y = z->parent->parent->right;
                    if (y && y->colour == RBColour::Red)
                    {
                         z->parent->colour = RBColour::Black;
                         y->colour = RBColour::Black;
                         z->parent->parent->colour = RBColour::Red;
                         z = z->parent->parent;
                    }
                    else
                    {
                         if (z == z->parent->right)
                         {
                              z = z->parent;
                              RotateLeft(z);
                         }

                         z->parent->colour = RBColour::Black;
                         z->parent->parent->colour = RBColour::Red;
                         RotateRight(z->parent->parent);
                    }
               }
               else
               {
                    VADNode* y = z->parent->parent->left;
                    if (y && y->colour == RBColour::Red)
                    {
                         z->parent->colour = RBColour::Black;
                         y->colour = RBColour::Black;
                         z->parent->parent->colour = RBColour::Red;
                         z = z->parent->parent;
                    }
                    else
                    {
                         if (z == z->parent->left)
                         {
                              z = z->parent;
                              RotateRight(z);
                         }

                         z->parent->colour = RBColour::Black;
                         z->parent->parent->colour = RBColour::Red;
                         RotateLeft(z->parent->parent);
                    }
               }
          }
          this->_root->colour = RBColour::Black;
     }

     bool VirtualMemoryAllocator::Insert(const VADEntry& entry)
     {
          if (FindOverlap(entry.baseAddress, entry.size)) return false;

          VADNode* z = _nodeAllocator.AllocateNode(entry);
          if (z == nullptr) return false;

          VADNode* y = nullptr;
          VADNode* x = _root;

          while (x)
          {
               y = x;
               if (z->entry.baseAddress < x->entry.baseAddress) x = x->left;
               else
                    x = x->right;
          }

          z->parent = y;

          if (y == nullptr) this->_root = z;
          else if (z->entry.baseAddress < y->entry.baseAddress)
               y->left = z;
          else
               y->right = z;

          UpdateMaxEndUpwards(z);

          FixInsert(z);

          return true;
     }

     VADNode* VirtualMemoryAllocator::Search(std::uintptr_t baseAddress)
     {
          VADNode* current = this->_root;

          while (current)
          {
               if (baseAddress == current->entry.baseAddress) return current;

               if (baseAddress < current->entry.baseAddress) current = current->left;
               else
                    current = current->right;
          }

          return nullptr;
     }

     VADNode* VirtualMemoryAllocator::FindOverlap(std::uintptr_t baseAddress, std::size_t size)
     {
          std::uintptr_t endAddress = baseAddress + size;
          VADNode* current = _root;

          while (current)
          {
               if (baseAddress < current->EndAddress() && endAddress > current->entry.baseAddress) return current;

               if (current->left && current->left->maxEnd > baseAddress) current = current->left;
               else
                    current = current->right;
          }

          return nullptr;
     }

     VADNode* VirtualMemoryAllocator::FindContaining(std::uintptr_t address)
     {
          VADNode* current = this->_root;

          while (current)
          {
               if (address >= current->entry.baseAddress && address < current->EndAddress()) return current;

               if (current->left && current->left->maxEnd > address) current = current->left;
               else
                    current = current->right;
          }

          return nullptr;
     }

     VADNode* VirtualMemoryAllocator::Minimum(VADNode* node)
     {
          while (node->left) node = node->left;
          return node;
     }

     void VirtualMemoryAllocator::Transplant(VADNode* u, VADNode* v)
     {
          if (!u->parent) _root = v;
          else if (u == u->parent->left)
               u->parent->left = v;
          else
               u->parent->right = v;

          if (v) v->parent = u->parent;
     }

     void VirtualMemoryAllocator::FixDelete(VADNode* x)
     {
          while (x != this->_root && (x == nullptr || x->colour == RBColour::Black))
          {
               if (x == x->parent->left)
               {
                    VADNode* w = x->parent->right;

                    if (w && w->colour == RBColour::Red)
                    {
                         w->colour = RBColour::Black;
                         x->parent->colour = RBColour::Red;
                         RotateLeft(x->parent);
                         w = x->parent->right;
                    }

                    if ((!w->left || w->left->colour == RBColour::Black) &&
                        (!w->right || w->right->colour == RBColour::Black))
                    {
                         w->colour = RBColour::Red;
                         x = x->parent;
                    }
                    else
                    {
                         if (!w->right || w->right->colour == RBColour::Black)
                         {
                              if (w->left) w->left->colour = RBColour::Black;
                              w->colour = RBColour::Red;
                              RotateRight(w);
                              w = x->parent->right;
                         }

                         w->colour = x->parent->colour;
                         x->parent->colour = RBColour::Black;
                         if (w->right) w->right->colour = RBColour::Black;
                         RotateLeft(x->parent);
                         x = _root;
                    }
               }
               else
               {
                    VADNode* w = x->parent->left;

                    if (w && w->colour == RBColour::Red)
                    {
                         w->colour = RBColour::Black;
                         x->parent->colour = RBColour::Red;
                         RotateRight(x->parent);
                         w = x->parent->left;
                    }

                    if ((!w->right || w->right->colour == RBColour::Black) &&
                        (!w->left || w->left->colour == RBColour::Black))
                    {
                         w->colour = RBColour::Red;
                         x = x->parent;
                    }
                    else
                    {
                         if (!w->left || w->left->colour == RBColour::Black)
                         {
                              if (w->right) w->right->colour = RBColour::Black;
                              w->colour = RBColour::Red;
                              RotateLeft(w);
                              w = x->parent->left;
                         }

                         w->colour = x->parent->colour;
                         x->parent->colour = RBColour::Black;
                         if (w->left) w->left->colour = RBColour::Black;
                         RotateRight(x->parent);
                         x = this->_root;
                    }
               }
          }

          if (x) x->colour = RBColour::Black;
     }

     bool VirtualMemoryAllocator::Remove(std::uintptr_t baseAddress)
     {
          VADNode* z = Search(baseAddress);
          if (z == nullptr) return false;

          VADNode* y = z;
          VADNode* x = nullptr;
          RBColour yOriginalColour = y->colour;
          VADNode* xParent = nullptr;

          if (!z->left)
          {
               x = z->right;
               xParent = z->parent;
               Transplant(z, z->right);
          }
          else if (!z->right)
          {
               x = z->left;
               xParent = z->parent;
               Transplant(z, z->left);
          }
          else
          {
               y = Minimum(z->right);
               yOriginalColour = y->colour;
               x = y->right;
               xParent = y;

               if (y->parent == z)
               {
                    if (x) x->parent = y;
               }
               else
               {
                    xParent = y->parent;
                    Transplant(y, y->right);
                    y->right = z->right;
                    y->right->parent = y;
               }

               Transplant(z, y);
               y->left = z->left;
               y->left->parent = y;
               y->colour = z->colour;
               UpdateMaxEnd(y);
          }

          if (xParent) UpdateMaxEndUpwards(xParent);

          if (yOriginalColour == RBColour::Black)
          {
               if (x != nullptr) FixDelete(x);
               else if (xParent)
                    FixDelete(xParent->left ? xParent->left : xParent->right);
          }

          this->_nodeAllocator.FreeNode(z);
          return true;
     }

     void VirtualMemoryAllocator::InorderTraversal(VADNode* node, void (*callback)(const VADEntry&))
     {
          if (!node) return;

          InorderTraversal(node->left, callback);
          callback(node->entry);
          InorderTraversal(node->right, callback);
     }

     bool VirtualMemoryAllocator::ReserveVirtualMemoryFixed(std::uintptr_t baseAddress, std::size_t size,
                                                            MemoryProtection protection, PFNUse use)
     {
          VADEntry entry(baseAddress, size, VADMemoryState::Reserved, protection, use);
          return Insert(entry);
     }

     std::uintptr_t VirtualMemoryAllocator::FindFreeRegion(std::size_t size, std::uintptr_t hint)
     {
          std::uintptr_t searchAddress = hint > 0 ? hint : _searchStart;

          if (this->_root == nullptr) return searchAddress;

          VADNode* current = this->_root;

          while (current->left) current = current->left;

          std::uintptr_t candidateAddress = searchAddress;

          while (current)
          {
               if (candidateAddress + size <= current->entry.baseAddress) return candidateAddress;

               candidateAddress = std::max(candidateAddress, current->entry.baseAddress + current->entry.size);

               if (current->right)
               {
                    current = current->right;
                    while (current->left) current = current->left;
               }
               else
               {
                    VADNode* parent = current->parent;
                    while (parent && current == parent->right)
                    {
                         current = parent;
                         parent = parent->parent;
                    }
                    current = parent;
               }
          }

          return candidateAddress;
     }

     std::uintptr_t VirtualMemoryAllocator::ReserveVirtualMemory(std::size_t size, MemoryProtection protection,
                                                                 PFNUse use)
     {
          std::uintptr_t baseAddress = FindFreeRegion(size, 0);
          if (baseAddress == 0) return 0;

          if (ReserveVirtualMemoryFixed(baseAddress, size, protection, use)) return baseAddress;

          return 0;
     }

     std::uintptr_t VirtualMemoryAllocator::ReserveVirtualMemoryAt(std::uintptr_t hint, std::size_t size,
                                                                   MemoryProtection protection, PFNUse use)
     {
          std::uintptr_t baseAddress = FindFreeRegion(size, hint);
          if (baseAddress == 0) return 0;

          if (ReserveVirtualMemoryFixed(baseAddress, size, protection, use)) return baseAddress;

          return 0;
     }
} // namespace memory
