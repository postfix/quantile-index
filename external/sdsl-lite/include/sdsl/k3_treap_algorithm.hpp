/* sdsl - succinct data structures library
    Copyright (C) 2014 Simon Gog

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .
*/
/*! \file k3_treap_algorithm.hpp
    \brief k3_treap_algorithm.hpp contains k^2-treap algorithms.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_K3_TREAP_ALGORITHM
#define INCLUDED_SDSL_K3_TREAP_ALGORITHM

#include "sdsl/vectors.hpp"
#include "sdsl/bits.hpp"
#include "sdsl/k3_treap_helper.hpp"
#include <tuple>
#include <algorithm>
#include <iterator>
#include <climits>
#include <vector>
#include <complex>
#include <queue>
#include <array>

//! Namespace for the succinct data structure library.
namespace sdsl
{

namespace k3_treap_ns
{

//! Check if point x is contained in the cuboid (p1,p2)
/*! \param p Point.
 *  \param Lower left corner of the rectangle.
 *  \param Upper right corner of the rectangle.
 */
bool
contained(const point_type p, const point_type& p1, const point_type& p2)
{
    return std::get<0>(p) >= std::get<0>(p1) and std::get<0>(p) <= std::get<0>(p2) and
           std::get<1>(p) >= std::get<1>(p1) and std::get<1>(p) <= std::get<1>(p2) and
           std::get<2>(p) >= std::get<2>(p1) and std::get<2>(p) <= std::get<2>(p2);
}

//! Check if the cuboid of node v is contained in the cuboid (p1,p2)
template<uint8_t t_k>
bool
contained(const point_type& p1, const point_type& p2, const node_type& v)
{
//    uint64_t d = (1ULL << v.t)-1;
//    uint64_t d = (1ULL << v.t)-1;
    uint64_t d = precomp<t_k>::exp(v.t)-1;
    return std::get<0>(p1) <= std::get<0>(v.p) and std::get<0>(p2) >= std::get<0>(v.p) + d and
           std::get<1>(p1) <= std::get<1>(v.p) and std::get<1>(p2) >= std::get<1>(v.p) + d and
           std::get<2>(p1) <= std::get<2>(v.p) and std::get<2>(p2) >= std::get<2>(v.p) + d;
}

//! Check if cuboid (p1,p2) and the cuboid of node v overlap
template<uint8_t t_k>
bool
overlap(const point_type& p1, const point_type& p2, const node_type& v)
{
//    uint64_t d = (1ULL << v.t)-1;
    uint64_t d = precomp<t_k>::exp(v.t)-1;
    return std::get<0>(p1) <= std::get<0>(v.p) + d and std::get<0>(p2) >= std::get<0>(v.p) and
           std::get<1>(p1) <= std::get<1>(v.p) + d and std::get<1>(p2) >= std::get<1>(v.p) and
           std::get<2>(p1) <= std::get<2>(v.p) + d and std::get<2>(p2) >= std::get<2>(v.p);
}

template <typename T>
struct persistent_prio_queue {
    std::shared_ptr<std::priority_queue<T>> m_pq;

    persistent_prio_queue() : m_pq(std::make_shared<std::priority_queue<T>>()) {}
    persistent_prio_queue(const persistent_prio_queue<T>& other) = default;
    persistent_prio_queue& operator=(const persistent_prio_queue<T>&) = default;

    void ensure_unique() {
        if (!m_pq.unique())
            m_pq = std::make_shared<std::priority_queue<T>>(*m_pq);
    }

    void push(const T& x) {
        ensure_unique();
        m_pq->push(x);
    }

    void pop() {
        ensure_unique();
        m_pq->pop();
    }

    bool empty() const { return m_pq->empty(); }
    size_t size() const { return m_pq->size(); }
    const T& top() const { return m_pq->top(); }
};

template<typename t_k3_treap>
class top_k_iterator
{

    public:
        typedef k3_treap_ns::node_type node_type;
        typedef void(*t_mfptr)();
        typedef std::tuple<point_type, uint64_t, node_type> t_point_val_node;

    public:
        const t_k3_treap* m_treap = nullptr;
        std::priority_queue<node_type> m_pq;
        t_point_val_node m_point_val_node;
        point_type m_p1;
        point_type m_p2;
        bool m_valid = false;

    public:
        top_k_iterator() = default;
        top_k_iterator(const top_k_iterator&) = default;
        top_k_iterator(top_k_iterator&&) = default;
        top_k_iterator& operator=(const top_k_iterator&) = default;
        top_k_iterator& operator=(top_k_iterator&&) = default;
        top_k_iterator(const t_k3_treap& treap, point_type p1, point_type p2) :
            top_k_iterator(treap,treap.root(),p1,p2) {

        }

        top_k_iterator(const t_k3_treap& treap, node_type v, point_type p1, point_type p2) :
            m_treap(&treap), m_p1(p1), m_p2(p2), m_valid(treap.size()>0)
        {
            //std::cout << "constructing [" << m_p1[0] << "," << m_p1[1] << "," << m_p1[2] << "] [" <<
                                        //m_p2[0] << "," << m_p2[1] << "," << m_p2[2] << "] " << std::endl;
            if (m_treap->size() > 0) { // TODO: check this condition for nodes
                m_pq.push(treap.root());
                ++(*this);
            }
        }

        std::array<top_k_iterator, 2> split() {
            //std::cout << "m_p1 = " << m_p1[0] <<  " , " << m_p1[1] << " , " << m_p1[2] << std::endl;
            //std::cout << "m_p2 = " << m_p2[0] <<  " , " << m_p2[1] << " , " << m_p2[2] << std::endl;
            //std::cout << "trying to split" << std::get<0>(m_point_val_node)[0] << "," << std::get<0>(m_point_val_node)[1] << "," << std::get<0>(m_point_val_node)[2] << std::endl;
            //std::cout << "m_pq.size = " << m_pq.size() << std::endl;
            std::array<top_k_iterator, 2> res;
//            if (m_pq.empty()) {
//                ++(*this);
//            }
            if (std::get<0>(m_point_val_node)[2] > 0) {
                // [ m_p1[2].....left[2] ]
                point_type p2 = m_p2;
                p2[2] = std::get<0>(m_point_val_node)[2] - 1;
                res[0] = top_k_iterator(*m_treap, std::get<2>(m_point_val_node), m_p1, p2); // optimize
            }
            if (std::get<0>(m_point_val_node)[2] < m_p2[2]) {
                // [ right......m_p2[2] ]
                point_type p1 = m_p1;
                p1[2] = std::get<0>(m_point_val_node)[2] +1;
                res[1] = top_k_iterator(*m_treap, std::get<2>(m_point_val_node), p1, m_p2);
            }

            return res;
        };

        size_t queue_size() const { return m_pq.size(); }
        uint64_t split_point() const { return (m_p1[2] + m_p2[2]) >> 1; }

        std::array<top_k_iterator, 2> split2() const {
            std::array<top_k_iterator, 2> res;
            uint64_t mid = split_point();
            uint64_t cur_z = std::get<0>(m_point_val_node)[2];
            {
                res[0] = *this;
                res[0].m_p2[2] = mid;
                if (cur_z > mid) ++res[0];
            }
            {
                res[1] = *this;
                res[1].m_p1[2] = mid + 1;
                if (cur_z <= mid) ++res[1];
            }
            return res;
        };

    //! Prefix increment of the iterator
        top_k_iterator& operator++()
        {
            //std::cout << "iterating... [" << m_p1[0] << "," << m_p1[1] << "," << m_p1[2] << "] [" <<
                //m_p2[0] << "," << m_p2[1] << "," << m_p2[2] << "] " << std::endl;

            //std::cout << "here????" << std::endl;
            m_valid = false;
            while (!m_pq.empty()) {
                //std::cout << "here????2" << std::endl;

                auto v = m_pq.top();
                m_pq.pop();

                if (overlap<t_k3_treap::k>(m_p1, m_p2, v)) {
                    for (auto w : m_treap->children(v))
                        if (overlap<t_k3_treap::k>(m_p1, m_p2, w))
                            m_pq.push(w);
                    if (contained(v.max_p, m_p1, m_p2)) {
                        //std::cout << "here????4" << std::endl;
                        m_point_val_node = t_point_val_node(v.max_p, v.max_v, v);
                        //std::cout << "point val = " << std::get<0>(m_point_val_node)[0] << "," << std::get<0>(m_point_val_node)[1] << "," << std::get<0>(m_point_val_node)[2] << std::endl;
                        m_valid = true;
                        break;
                    }
                }
            }
            return *this;
        }

        //! Postfix increment of the iterator
        top_k_iterator operator++(int)
        {
            top_k_iterator it = *this;
            ++(*this);
            return it;
        }

        t_point_val_node operator*() const
        {
//            std::cout << "returning point " << std::get<0>(m_point_val_node)[0] << "," << std::get<0>(m_point_val_node)[1] << "," <<  std::get<0>(m_point_val_node)[2] << std::endl;
            return m_point_val_node;
        }

        bool is_valid() const {
            return m_valid;
        }

        bool is_initialized() const {
            return m_treap;
        }

        //! Cast to a member function pointer
        // Test if there are more elements
        // Can be casted to bool but not implicit in an arithmetic experession
        // See Alexander C.'s comment on
        // http://stackoverflow.com/questions/835590/how-would-stdostringstream-convert-to-bool
        operator t_mfptr() const
        {
            return (t_mfptr)(m_valid);
        }

};

template<typename t_k3_treap>
class range_iterator
{
    public:
        typedef void(*t_mfptr)();
        typedef std::pair<point_type, uint64_t> t_point_val;

    private:
        typedef k3_treap_ns::node_type node_type;
        typedef std::pair<node_type, bool> t_nt_b;

        const t_k3_treap* m_treap = nullptr;
        std::priority_queue<t_nt_b> m_pq;
        t_point_val m_point_val;
        point_type m_p1;
        point_type m_p2;
        range_type m_r;
        bool m_valid = false;

        void pq_emplace(node_type v, bool b)
        {
            if (v.max_v >= std::get<0>(m_r)) {
                m_pq.emplace(v, b);
            }
        }

    public:
        range_iterator() = default;
        range_iterator(const range_iterator&) = default;
        range_iterator(range_iterator&&) = default;
        range_iterator& operator=(const range_iterator&) = default;
        range_iterator& operator=(range_iterator&&) = default;
        range_iterator(const t_k3_treap& treap, point_type p1, point_type p2, range_type range) :
            m_treap(&treap), m_p1(p1), m_p2(p2), m_r(range), m_valid(treap.size()>0)
        {
            if (m_treap->size() >0) {
                pq_emplace(m_treap->root(), false);
                ++(*this);
            }
        }

        //! Prefix increment of the iterator
        range_iterator& operator++()
        {
            m_valid = false;
            while (!m_pq.empty()) {
                auto v = std::get<0>(m_pq.top());
                auto is_contained = std::get<1>(m_pq.top());
                m_pq.pop();
                if (is_contained) {
                    auto nodes = m_treap->children(v);
                    for (auto node : nodes)
                        pq_emplace(node, true);
                    if (v.max_v <= std::get<1>(m_r)) {
                        m_point_val = t_point_val(v.max_p, v.max_v);
                        m_valid = true;
                        break;
                    }
                } else {
                    if (contained<t_k3_treap::k>(m_p1, m_p2, v)) {
                        m_pq.emplace(v, true);
                    } else if (overlap<t_k3_treap::k>(m_p1, m_p2, v)) {
                        auto nodes = m_treap->children(v);
                        for (auto node : nodes)
                            pq_emplace(node, false);
                        if (contained(v.max_p, m_p1, m_p2) and v.max_v <= std::get<1>(m_r)) {
                            m_point_val = t_point_val(v.max_p, v.max_v);
                            m_valid = true;
                            break;
                        }
                    }
                }
            }
            return *this;
        }

        //! Postfix increment of the iterator
        range_iterator operator++(int)
        {
            range_iterator it = *this;
            ++(*this);
            return it;
        }

        t_point_val operator*() const
        {
            return m_point_val;
        }

        //! Cast to a member function pointer
        // Test if there are more elements
        operator t_mfptr() const
        {
            return (t_mfptr)(m_valid);
        }
};

//! Get iterator for all heaviest points in rectangle (p1,p2) in decreasing order
/*! \param treap k2-treap
 *  \param p1    Lower left corner of the rectangle
 *  \param p2    Upper right corner of the rectangle
 *  \return Iterator to result in decreasing order.
 *  \pre std::get<0>(p1) <= std::get<0>(p2) and std::get<1>(p1)<=std::get<1>(p2)
 */
template<typename t_k3_treap>
k3_treap_ns::top_k_iterator<t_k3_treap>
top_k(const t_k3_treap& t,
      k3_treap_ns::point_type p1,
      k3_treap_ns::point_type p2)
{
    return k3_treap_ns::top_k_iterator<t_k3_treap>(t, p1, p2);
}

//! Get iterator for all points in rectangle (p1,p2) with weights in range
/*! \param treap k2-treap
 *  \param p1    Lower left corner of the rectangle
 *  \param p2    Upper right corner of the rectangle
 *  \param range Range {w1,w2}.
 *  \return Iterator to list of all points in the range.
 *  \pre std::get<0>(p1) <= std::get<0>(p2) and std::get<1>(p1)<=std::get<1>(p2)
 *       std::get<0>(range) <= std::get<1>(range)
 */
template<typename t_k3_treap>
k3_treap_ns::range_iterator<t_k3_treap>
range_3d(const t_k3_treap& t,
         k3_treap_ns::point_type p1,
         k3_treap_ns::point_type p2,
         k3_treap_ns::range_type range)
{
    return k3_treap_ns::range_iterator<t_k3_treap>(t, p1, p2, range);
}


// forward declaration
template<typename t_k3_treap>
uint64_t __count(const t_k3_treap&, typename t_k3_treap::node_type);

// forward declaration
template<typename t_k3_treap>
uint64_t _count(const t_k3_treap&, k3_treap_ns::point_type,
                k3_treap_ns::point_type, typename t_k3_treap::node_type);

//! Count how many points are in the rectangle (p1,p2)
/*! \param treap k2-treap
 *  \param p1    Lower left corner of the rectangle.
 *  \param p2    Upper right corner of the rectangle.
 *  \return The number of points in rectangle (p1,p2).
 *  \pre std::get<0>(p1) <= std::get<0>(p2) and std::get<1>(p1)<=std::get<1>(p2)
 */
template<typename t_k3_treap>
uint64_t
count(const t_k3_treap& treap,
      k3_treap_ns::point_type p1,
      k3_treap_ns::point_type p2)
{
    if (treap.size() > 0) {
        return _count(treap, p1, p2, treap.root());
    }
    return 0;
}


template<typename t_k3_treap>
uint64_t
_count(const t_k3_treap& treap,
       k3_treap_ns::point_type p1,
       k3_treap_ns::point_type p2,
       typename t_k3_treap::node_type v)
{
    using namespace k3_treap_ns;
    if (contained<t_k3_treap::k>(p1, p2, v)) {
        return __count(treap, v);
    } else if (overlap<t_k3_treap::k>(p1, p2, v)) {
        uint64_t res = contained(v.max_p, p1, p2);
        auto nodes = treap.children(v);
        for (auto node : nodes) {
            res += _count(treap, p1, p2, node);
        }
        return res;
    }
    return 0;
}


template<typename t_k3_treap>
uint64_t
__count(const t_k3_treap& treap,
        typename t_k3_treap::node_type v)
{
    uint64_t res = 1; // count the point at the node
    auto nodes = treap.children(v);
    for (auto node : nodes)
        res += __count(treap, node);
    return res;
}

} // namespace k3_treap_ns

// forward declaration
template<uint8_t  t_k,
         typename t_bv,
         typename t_rank,
         typename t_max_vec>
class k3_treap;


//! Specialized version of method ,,construct'' for k3_treaps.
template<uint8_t  t_k,
         typename t_bv,
         typename t_rank,
         typename t_max_vec>
void
construct(k3_treap<t_k, t_bv, t_rank, t_max_vec>& idx, std::string file)
{
    int_vector_buffer<> buf_x(file+".x", std::ios::in);
    int_vector_buffer<> buf_y(file+".y", std::ios::in);
    int_vector_buffer<> buf_z(file+".z", std::ios::in);
    int_vector_buffer<> buf_w(file+".w", std::ios::in);
    k3_treap<t_k, t_bv, t_rank, t_max_vec> tmp(buf_x, buf_y, buf_z, buf_w);
    tmp.swap(idx);
}

//! Specialized version of method ,,construct_im'' for k3_treaps.
template<uint8_t  t_k,
         typename t_bv,
         typename t_rank,
         typename t_max_vec
         >
void
construct_im(k3_treap<t_k, t_bv, t_rank, t_max_vec>& idx, std::vector<std::array<uint64_t, 4>> data)
{
    std::string tmp_prefix = ram_file_name("k3_treap_");
    std::vector<std::tuple<uint64_t,uint64_t,uint64_t,uint64_t>> d;
    for (auto x : data) {
        d.push_back(std::make_tuple(x[0],x[1],x[2],x[3]));
    }
    k3_treap<t_k, t_bv, t_rank, t_max_vec> tmp(d, tmp_prefix);
    tmp.swap(idx);
}

} // namespace sdsl
#endif


