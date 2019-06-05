// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DB_ACCESS_H
#define PERSIST_DB_ACCESS_H

#include <tuple>

#include "dbconf.h"
#include "leveldbwrapper.h"

/**
 * Empty functions
 */
namespace db_util {
    inline bool IsEmpty(const int val) { return true;}

    // string
    template<typename C> bool IsEmpty(const basic_string<C> &val);
    template<typename C> void SetEmpty(basic_string<C> &val);

    // vector
    template<typename T, typename A> bool IsEmpty(const vector<T, A>& val);
    template<typename T, typename A> void SetEmpty(vector<T, A>& val);

    // 2 pair
    template<typename K, typename T> bool IsEmpty(const std::pair<K, T>& val);
    template<typename K, typename T> void SetEmpty(std::pair<K, T>& val);

    // 3 tuple
    template<typename T0, typename T1, typename T2> bool IsEmpty(const std::tuple<T0, T1, T2>& val);
    template<typename T0, typename T1, typename T2> void SetEmpty(std::tuple<T0, T1, T2>& val);

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T> bool IsEmpty(const T& val);
    template<typename T> void SetEmpty(const T& val);

    // string
    template<typename C>
    bool IsEmpty(const basic_string<C> &val) {
        return val.empty();
    }

    template<typename C>
    void SetEmpty(basic_string<C> &val) {
        return val.clear();
    }

    // vector
    template<typename T, typename A> 
    bool IsEmpty(const vector<T, A>& val) {
        return val.empty();
    }
    template<typename T, typename A> 
    void SetEmpty(vector<T, A>& val) {
        return val.clear();
    }

    // 2 pair
    template<typename K, typename T> 
    bool IsEmpty(const std::pair<K, T>& val) {
        return IsEmpty(val->first) && IsEmpty(val->second);
    }
    template<typename K, typename T> 
    void SetEmpty(std::pair<K, T>& val) {
        SetEmpty(val->first);
        SetEmpty(val->second);
    }

    // 3 tuple
    template<typename T0, typename T1, typename T2>
    bool IsEmpty(const std::tuple<T0, T1, T2>& val) {
        return IsEmpty(std::get<0>(val)) &&
               IsEmpty(std::get<1>(val)) &&
               IsEmpty(std::get<2>(val));
    }
    template<typename T0, typename T1, typename T2>
    void SetEmpty(std::tuple<T0, T1, T2>& val) {
        SetEmpty(std::get<0>(val));
        SetEmpty(std::get<1>(val));
        SetEmpty(std::get<2>(val));
    }

    // common Object Type, must support T.IsEmpty() and T.SetEmpty()
    template<typename T>
    bool IsEmpty(const T& val) {
        return val.IsEmpty();
    }

    template<typename T>
    void SetEmpty(const T& val) {
        return val.SetEmpty();
    }
};

class CDBAccess {
public:
    CDBAccess(const string &name, size_t nCacheSize, bool fMemory, bool fWipe) :
                db( GetDataDir() / "blocks" / name, nCacheSize, fMemory, fWipe ) {}

    template<typename KeyType, typename ValueType>
    bool GetData(dbk::PrefixType prefixType, const KeyType &key, ValueType &value) {
        return false;
        // string keyStr = dbk::GenDbKey(prefixType, key);
        // return db.Read(keyStr, value);
    }

    template<typename KeyType, typename ValueType>
    bool HaveData(dbk::PrefixType prefixType, const KeyType &key) {
        string keyStr = GenDbKey(prefixType, key);
        return db.Exists(keyStr);
    }


    template<typename KeyType, typename ValueType>
    void BatchWrite(dbk::PrefixType prefixType, const map<KeyType, ValueType> &mapData) {
        CLevelDBBatch batch;
        for (auto it : mapData) {
            if (db_util::IsEmpty(it->second)) {
                batch.Erase(dbk::GenDbKey(prefixType, it->first));
            } else {
                batch.Write(dbk::GenDbKey(prefixType, it->first));
            }
        }
        db.WriteBatch(batch, true);
    }

private:
    CLevelDBWrapper db;
};

template<typename KeyType, typename ValueType>
class CDBCache {

public:
    typedef typename map<KeyType, ValueType>::iterator Iterator;
public:
    /**
     * Default constructor, must use set base to initialize before using.
     */
    CDBCache(): pBase(nullptr), pDbAccess(nullptr), prefixType(dbk::EMPTY) {};

    CDBCache(CDBCache<KeyType, ValueType> *pBaseIn): pBase(pBaseIn), 
        pDbAccess(nullptr), prefixType(pBaseIn->prefixType) {
        assert(pBaseIn != nullptr);
    };

    CDBCache(CDBAccess *pDbAccessIn, dbk::PrefixType prefixTypeIn): pBase(nullptr), 
        pDbAccess(pDbAccessIn), prefixType(prefixTypeIn) { 
        assert(pDbAccessIn != nullptr); 
    };  
    void SetBase(CDBCache<KeyType, ValueType> *pBaseIn) {
        assert(pDbAccess == nullptr);
        assert(mapData.empty());
        pBase = pBaseIn;
        prefixType = pBaseIn->prefixType;
    };

    bool GetData(const KeyType &key, ValueType &value) {
        auto it = GetDataIt(key);
        if (it != mapData.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    bool SetData(const KeyType &key, const ValueType &value) {
        if (db_util::IsEmpty(key)) {
            return false;
        }
        mapData[key] = value;
        return true;
    };


    bool HaveData(const KeyType &key) {
        // TODO: need to implements with pDbAccess.HaveData() ?
        auto it = GetDataIt(key);
        return it != mapData.end();
    }

    bool EraseData(const KeyType &key) {
        auto it = GetDataIt(key);
        if (it != mapData.end()) {
            db_util::SetEmpty(it->second);
        }
        return true;
    }

    void Flush() {
        assert(pBase != nullptr || pDbAccess != nullptr);
        if (pBase != nullptr) {
            assert(pDbAccess == nullptr);
            for (auto it : this.mapdata) {
                pBase->mapData[it.first] = it.second;
            }
            this.mapData.clear();
        } else if (pDbAccess != nullptr) {
            assert(pBase == nullptr);
            pDbAccess->BatchWrite(prefixType, mapData);
        }

        this.mapData.clear();
    }

    dbk::PrefixType GetPrefixType() { return prefixType; }

private:
    Iterator GetDataIt(const KeyType &key) {
        // key should not be empty
        if (db_util::IsEmpty(key)) {
            return mapData.end();
        }

        Iterator it = mapData.find(key);
        if (it != mapData.end()) {
            if (!db_util::IsEmpty(it->second)) {
                return it;
            }
        } else if (pBase != nullptr){
            auto it = pBase->GetDataIt(key);
            if (it != mapData.end()) {
                auto newIt = mapData.emplace(make_pair(key, it->second));
                assert(newIt.second); // TODO: throw error
                return newIt.first;
            }
        } else if (pDbAccess != NULL) {
            auto ptrValue = std::make_shared<ValueType>();
            
            if (pDbAccess->GetData(prefixType, key, *ptrValue)) {
                auto newIt = mapData.emplace(make_pair(key, *ptrValue));
                assert(newIt.second); // TODO: throw error
                return newIt.first;
            }
        }

        return mapData.end();
    };
private:
    CDBCache<KeyType, ValueType> *pBase;
    CDBAccess *pDbAccess;
    dbk::PrefixType prefixType;
    map<KeyType, ValueType> mapData;

};

#endif//PERSIST_DB_ACCESS_H