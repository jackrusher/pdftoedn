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

#pragma once

#include <vector>

#ifdef CHECK_CAP_CHANGE
#include <iostream>
#include <assert.h>
// just something to make sure sized EDN vectors and hashes don't
// realloc
#define TRACE_CAPACITY_CHANGE( push ) {                       \
        uintmax_t c1 = elems->capacity();                     \
        push;                                                 \
        uintmax_t c2 = elems->capacity();                     \
        bool container_cap_changed = (c1 != c2 &&             \
                                      (c1 != 0 && c2 != 1));  \
        if (container_cap_changed) {                          \
            std::cerr << "CAP changed from " << c1 << " to "  \
                      << c2 << std::endl;                     \
            assert(container_cap_changed);                    \
        }                                                     \
    }
#else
#define TRACE_CAPACITY_CHANGE( push ) push;
#endif

#include "base_types.h"


namespace pdftoedn {
    namespace util {
        namespace edn {

            class EDNNode;

            // ===========================================================
            // generic container to represent EDN vector and hash
            // sequences. Vectors behave mostly as expected but Hashes
            // don't function as a real hash/map as they are also
            // represented as a vector (since the main purpose of the
            // EDN Hash is to output a sequence of pairs. For this
            // reason, util::edn::Vector and util::edn::Hash should
            // not be thought of as std::vector and std::unordered_map
            // - they won't work the same way.
            //
            // I initially thought of represent these using those
            // std:: types with elements as std::shared_ptr but I
            // tested switching everything and performance took a
            // serious hit. For simplification and speed, this does
            // the job at the expense of requiring minor extra care to
            // know you can't replace a entry in a util::edn::Hash
            // since it's just a vector.
            //
            // Also note that these use EDNNodes to store the
            // data. This means:
            //
            // * "primitive" and standard types (int, uintmax_t, bool,
            // double) are stored as a copy
            //
            // * a std::string is allocated to store char * but
            //   std::strings are only copied if passed as an r-value
            //
            // * Coord and BoundingBox storage behavior is the same as
            //   std::string
            //
            // * any 'edn' type passed as a pointer is not copied
            //   since it is assumed they are members of a class that
            //   will persist beyond the call to to_edn().
            //
            template <class T>
            class Container : public pdftoedn::gemable {
            protected:
                Container() : elems(nullptr) {}
                Container(uintmax_t size) : elems(new std::vector<T>()) { elems->reserve(size); }
            public:
                Container(Container<T>& v) : elems(nullptr) {
                    swap(*this, v);
                }
                Container(Container<T>&& v) : elems(nullptr) {
                    swap(*this, v);
                }
                Container<T>& operator=(Container<T> v) {
                    swap(*this, v);
                    return *this;
                }
                virtual ~Container() { delete elems; }

                bool empty() const { return (!elems || elems->empty()); }
                void reserve(uintmax_t size) {
                    if (!elems) elems = new std::vector<T>;
                    // increase capacity by the given size
                    elems->reserve(size + elems->capacity());
                }
                virtual std::ostream& to_edn(std::ostream& o) const;

                friend void swap(Container<T>& c1, Container<T>& c2) {
                    using std::swap;
                    swap(c1.elems, c2.elems);
                }

            protected:
                void push_elem(const T& n) {
                    if (!elems) {
                        elems = new std::vector<T>;
                    }
                    TRACE_CAPACITY_CHANGE(
                    elems->push_back(n);
                    );
                }

            private:
                std::vector<T>* elems;

                virtual const char* sep_chars() const = 0;
                virtual const char* open_chars() const = 0;
                virtual const char* close_chars() const = 0;
            };

            // ===========================================================
            // util::edn::Vector is a Container of EDNNodes so the
            // interface is 1-1. We expect to know the size of the
            // container so the constructor requires it
            struct Vector : public Container<EDNNode>
            {
                Vector(uintmax_t size) : Container<EDNNode>(size) {}

                void push(const EDNNode& n) {
                    push_elem(n);
                }

                virtual const char* sep_chars() const { return " "; }
                virtual const char* open_chars() const { return "["; }
                virtual const char* close_chars() const { return "]"; }
            };

            // ===========================================================
            // util::edn::Hash is a Container of EDNNode pairs. Size
            // is optional as there are cases where only one element
            // is pushed. The push interface creates the pair to be
            // stored
            struct Hash : public Container<std::pair<EDNNode, EDNNode> >
            {
                Hash() {}
                Hash(uintmax_t size) : Container<std::pair<EDNNode, EDNNode> >(size) {}
                void push(const EDNNode& n1, const EDNNode& n2);

                virtual const char* sep_chars() const { return ", "; }
                virtual const char* open_chars() const { return "{"; }
                virtual const char* close_chars() const { return "}"; }
            };


            // ===========================================================
            // the EDN Node proxy class that handles duplication and
            // freeing as necessary. It is similar to unique_ptr in
            // that a node holds ownership until it is copied except
            // for primitives (bool, intmax_t, etc.) for which the
            // value is stored directly. Ownership is transfered when
            // copied, moved, or assigned. Owner nodes delete their
            // elements.
            class EDNNode
            {
            public:

                enum Type { UVAL_BOOL, UVAL_UINT, UVAL_INT, UVAL_DOUBLE, UVAL_STRING, UVAL_OBJ };

                // copy & move constructors + op= transfer
                // ownership. n becomes invalid
                explicit EDNNode(const EDNNode& n) :
                    type(n.type), val(n.val), owner(n.owner) {
                    n.owner = false;
                }
                EDNNode(EDNNode&& n) :
                    type(n.type), val(n.val), owner(n.owner) {
                    n.owner = false;
                }
                EDNNode& operator=(EDNNode);
                EDNNode() = delete;
                ~EDNNode();

                // all these are stored directly - no object is created
                EDNNode(bool v)      : type(UVAL_BOOL),   val(v), owner(false) {}
                EDNNode(uintmax_t v) : type(UVAL_UINT),   val(v), owner(false) {}
                EDNNode(uint8_t v)   : type(UVAL_UINT),   val(v), owner(false) {}
                EDNNode(intmax_t v)  : type(UVAL_INT),    val(v), owner(false) {}
                EDNNode(int v)       : type(UVAL_INT),    val(v), owner(false) {}
                EDNNode(double v)    : type(UVAL_DOUBLE), val(v), owner(false) {}
                // a std::string instance is allocated to store a copy of
                // char *
                EDNNode(const char* v)        : type(UVAL_STRING), val(new std::string(v)), owner(true) {}
                // std::string saves a ptr when passed as an lvalue; otherwise, a copy is made
                EDNNode(const std::string& v) : type(UVAL_STRING), val(&v), owner(false) {}
                EDNNode(std::string&& v)      : type(UVAL_STRING), val(new std::string(v)), owner(true) {}
                // Coord, BoundingBox, Symbols can be passed as either
                // an lval or an rval. If the latter, a copy is
                // made. Bounds so far are only used as lvalues
                EDNNode(const pdftoedn::Coord& c);
                EDNNode(pdftoedn::Coord&& c);
                EDNNode(const pdftoedn::BoundingBox& b);
                EDNNode(pdftoedn::BoundingBox&& b);
                EDNNode(const pdftoedn::Bounds& b);
                EDNNode(const pdftoedn::Symbol& s);
                EDNNode(pdftoedn::Symbol&& s);
                // Vector and Hash are also copied but element sequences
                // are shallow copied
                EDNNode(Vector& v);
                EDNNode(Hash& h);
                // gemables passed as pointers store a copy of the pointer so
                // no deletion is done
                EDNNode(const pdftoedn::gemable* g) :
                    type(UVAL_OBJ), val(g), owner(false)
                { }

                std::ostream& to_edn(std::ostream& o) const;
                friend std::ostream& operator<<(std::ostream& o, const EDNNode& n) {
                    n.to_edn(o);
                    return o;
                }

            private:

                Type type;
                union Val {
                    Val(bool val)                     : b(val)   {}
                    Val(uintmax_t val)                : ui(val)  {}
                    Val(uint8_t val)                  : ui(val)  {}
                    Val(intmax_t val)                 : i(val)   {}
                    Val(int val)                      : i(val)   {}
                    Val(double val)                   : d(val)   {}
                    Val(const std::string* val)       : str(val) {}
                    Val(const pdftoedn::gemable* val) : obj(val) {}

                    bool b;
                    uintmax_t ui;
                    intmax_t i;
                    double d;
                    const std::string* str;
                    const pdftoedn::gemable* obj;
                } val;
                mutable bool owner;
            };
        }
    }
}

