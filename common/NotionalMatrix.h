//
// Created by vytautas on 9/8/17.
//

#ifndef SGX_NETTING_NOTIONALMATRIX_H
#define SGX_NETTING_NOTIONALMATRIX_H

#include <string>
#include <set>
#include <map>
#include <vector>
#include <cassert>
#include "shared_ptr.h"

#include "trade.h"

using namespace std;

struct NotionalMatrix {

    typedef std::pair<party_id_t, party_id_t> p_cp_t; // party, counter-party pair
    typedef map<p_cp_t, value_t> TradeMat;
    TradeMat matrix;
    typedef TradeMat::value_type MatVal;

    set<party_id_t> members_set;

    int zeros() const {
        return n_trade_pairs() - matrix.size();
    }

    int n_trade_pairs() const {
        return members_set.size()*(int(members_set.size()) - 1);
    }

    void add(const ClearedTrade& t) {
        members_set.insert(t.party);
        members_set.insert(t.counter_party);

        if(t.value == 0)
            return;

        p_cp_t p_cp_key(t.party, t.counter_party);
        p_cp_t cp_p_key(t.counter_party, t.party);

        auto it1 = matrix.find(p_cp_key);
        if(it1 == matrix.end())
            it1 = matrix.insert(MatVal(p_cp_key, 0)).first;
        else{
            if(it1->second == t.value)
                return;
            else
                throw runtime_error(string(
            "Notional trade already exists with different value:") + t.party + " <> " +t.counter_party);
        }

        auto it2 = matrix.find(cp_p_key);
        if(it2 == matrix.end())
            it2 = matrix.insert(MatVal(cp_p_key, 0)).first;

        it1->second += t.value;
        it2->second -= t.value;

        if(it1->second == 0)
            matrix.erase(it1);
        if(it2->second == 0)
            matrix.erase(it2);
    }

    void add(const vector<ClearedTrade>& trades ) {
        for(auto& t : trades)
            add(t);
    }

    vector<party_id_t> members_list() const {
        return vector<party_id_t>(members_set.begin(), members_set.end());
    }

    value_t operator()(party_id_t a, party_id_t b) const {
        assert(members_set.count(a) && members_set.count(b) && "Trade parties must exist.");
        auto it = matrix.find(p_cp_t(a,b));
        if(it != matrix.end() )
            return it->second;
        else
            return 0;
    }

    void put(party_id_t a, party_id_t b, value_t val) {
        assert(members_set.count(a) && members_set.count(b) && "Trade parties must exist.");

        p_cp_t p_cp_key(a,b);
        p_cp_t cp_p_key(b,a);

        auto it1 = matrix.find(p_cp_key);
        if(it1 == matrix.end())
            it1 = matrix.insert(MatVal(p_cp_key, 0)).first;
        auto it2 = matrix.find(cp_p_key);
        if(it2 == matrix.end())
            it2 = matrix.insert(MatVal(cp_p_key, 0)).first;

        it1->second += val;
        it2->second -= val;

        if(it1->second == 0)
            matrix.erase(it1);
        if(it2->second == 0)
            matrix.erase(it2);
    }

    vector<ClearedTrade> to_list() const {
        vector<ClearedTrade> ret;
        auto m_list = members_list();
        for(int i=0; i< m_list.size(); i++)
            for(int j=i+1; j < m_list.size(); j++) {
                auto& a = m_list[i];
                auto& b = m_list[j];
                value_t val = (*this)(a,b);

                ClearedTrade t;
                t.party = a;
                t.counter_party = b;
                t.value = val;
                ret.push_back(t);
            }

        return ret;
    }

    vector<ClearedTrade> sub(const NotionalMatrix& rhs) {
        vector<ClearedTrade> ret;
        auto m_list = members_list();
        for(int i=0; i< m_list.size(); i++)
            for(int j=i+1; j < m_list.size(); j++) {
                auto& a = m_list[i];
                auto& b = m_list[j];
                value_t val = (*this)(a,b) - rhs(a,b);
                if(val != 0){
                    ClearedTrade t;
                    t.party = a;
                    t.counter_party = b;
                    t.value = val;
                    ret.push_back(t);
                }
            }

        return ret;
    }
};

#endif //SGX_NETTING_NOTIONALMATRIX_H
