//
// Copyright 2016-2019 Ed Porras
//
// This file is part of pdftoedn.
//
// pdftoedn is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdftoedn is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdftoedn.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ostream>
#include <vector>
#include <assert.h>

#include "base_types.h"
#include "util_edn.h"

namespace pdftoedn
{
    namespace util
    {
        namespace edn
        {
            // =============================================
            // output operator for pairs
            //
            std::ostream& operator<<(std::ostream& o, const std::pair<EDNNode,EDNNode>& p) {
                // hash pairs are separated by a space
                o << p.first << " " << p.second;
                return o;
            }

            // =============================================
            // Node proxy class to manage storage for EDN output to an
            // ostream
            //
            EDNNode::EDNNode(const pdftoedn::Coord& c) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&c)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::Coord&& c) :
                type(UVAL_OBJ), val(new pdftoedn::Coord(c)), owner(true) {
            }
            EDNNode::EDNNode(const pdftoedn::BoundingBox& b) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&b)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::BoundingBox&& b) :
                type(UVAL_OBJ), val(new pdftoedn::BoundingBox(b)), owner(true) {
            }
            EDNNode::EDNNode(const pdftoedn::Bounds& b) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&b)), owner(false) {
            }
            EDNNode::EDNNode(const pdftoedn::Symbol& v) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&v)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::Symbol&& v) :
                type(UVAL_OBJ), val(new pdftoedn::Symbol(v)), owner(true) {
            }
            EDNNode::EDNNode(Vector& v) :
                type(UVAL_OBJ), val(new Vector(v)), owner(true) {
            }
            EDNNode::EDNNode(Hash& h) :
                type(UVAL_OBJ), val(new Hash(h)), owner(true) {
            }
            EDNNode& EDNNode::operator=(EDNNode n) {
                type = n.type;
                val = n.val;
                owner = n.owner;
                n.owner = false;
                return *this;
            }
            EDNNode::~EDNNode() {
                if (owner) {
                    switch (type)
                    {
                      case UVAL_STRING: delete val.str; break;
                      case UVAL_OBJ:    delete val.obj; break;
                      default:
                          std::cerr << "ERROR: attempt to delete node of type: "
                                    << type << std::endl;
                          assert(0 && "EDNNode cleanup error");
                          break;
                    }
                }
            }
            std::ostream& EDNNode::to_edn(std::ostream& o) const {
                switch (type)
                {
                  case UVAL_BOOL:   o << std::boolalpha << val.b;   break;
                  case UVAL_UINT:   o << std::dec << val.ui;        break;
                  case UVAL_INT:    o << std::dec << val.i;         break;
                  case UVAL_DOUBLE: o << std::dec << val.d;         break;
                  case UVAL_OBJ:    o << *(val.obj);                break;
                  case UVAL_STRING:
                      o << '"';
                      // need to escape double quotes in the string
                      for (char c : (*val.str)) {
                          switch (c) {
                            case 0:
                            case '"':
                            case '\\':
                                o << '\\';
                                break;
                          }
                          o << c;
                      }
                      o << '"';
                      break;
                  default:
                      assert(0 && "attempt to output UNDEF node");
                      break;
                }
                return o;
            }

            // =============================================
            // EDN output sequence container
            //
            template <class T>
            std::ostream& Container<T>::to_edn(std::ostream& o) const
            {
                o << open_chars();
                if (elems) {
                    if (elems->size() > 1) {
                        // output each node with a space in between up
                        // to the penultimate one
                        std::for_each( elems->begin(), elems->end() - 1,
                                       [&](const T& n) {
                                           o << n << sep_chars();
                                       });
                    }
                    if (!elems->empty()) {
                        o << elems->back();
                    }
                }
                o << close_chars();
                return o;
            }

            // =============================================
            // Hash's push cerates the pair to be stored
            void Hash::push(const EDNNode& n1, const EDNNode& n2) {
                push_elem(std::pair<EDNNode, EDNNode>(n1, n2));
            }
        }
    }
} // namespace

