// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "chainparams.h"
#include "crypto/hash.h"
#include "id.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "vote.h"

using namespace json_spirit;



class CAccountLog;

class CAccount {
public:
    CRegID regID;                   //!< regID of the account
    CKeyID keyID;                   //!< keyID of the account (interchangeable to address)
    CPubKey pubKey;                 //!< public key of the account
    CPubKey minerPubKey;            //!< miner public key of the account
    CAccountNickID nickID;          //!< Nickname ID of the account (maxlen=32)

    uint64_t bcoinBalance;          //!< BaseCoin balance
    uint64_t scoinBalance;          //!< StableCoin balance
    uint64_t fcoinBalance;          //!< FundCoin balance

    uint64_t nVoteHeight;           //!< account vote block height
    vector<CVoteFund> vVoteFunds;   //!< account delegates votes sorted by vote amount
    uint64_t receivedVotes;         //!< votes received

    bool hasOpenCdp;                //!< When true, its CDP exists in a map {cdp-$regid -> $cdp}

    uint256 sigHash;                //!< memory only

public:
    /**
     * @brief operate account
     * @param type: operate type
     * @param values
     * @param nCurHeight:  the tip block height
     * @return returns true if successful, otherwise false
     */
    bool OperateAccount(OperType type, const uint64_t &values, const uint64_t nCurHeight);
    bool UndoOperateAccount(const CAccountLog &accountLog);
    bool ProcessDelegateVote(vector<COperVoteFund> &operVoteFunds, const uint64_t nCurHeight);
    bool OperateVote(VoteOperType type, const uint64_t &values);

public:
    CAccount(CKeyID &keyId, CPubKey &pubKey, CAccountNickID &nickId)
        : keyID(keyId),
          pubKey(pubKey),
          nickID(nickId),
          bcoinBalance(0),
          scoinBalance(0),
          fcoinBalance(0),
          nVoteHeight(0),
          receivedVotes(0) {
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount()
        : keyID(uint160()),
          bcoinBalance(0),
          scoinBalance(0),
          fcoinBalance(0),
          nVoteHeight(0),
          receivedVotes(0) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount(const CAccount &other) {
        this->regID         = other.regID;
        this->keyID         = other.keyID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoinBalance  = other.bcoinBalance;
        this->scoinBalance  = other.scoinBalance;
        this->fcoinBalance  = other.fcoinBalance;
        this->nVoteHeight   = other.nVoteHeight;
        this->vVoteFunds    = other.vVoteFunds;
        this->receivedVotes = other.receivedVotes;
    }

    CAccount &operator=(const CAccount &other) {
        if (this == &other)
            return *this;

        this->regID         = other.regID;
        this->keyID         = other.keyID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoinBalance  = other.bcoinBalance;
        this->scoinBalance  = other.scoinBalance;
        this->fcoinBalance  = other.fcoinBalance;
        this->nVoteHeight   = other.nVoteHeight;
        this->vVoteFunds    = other.vVoteFunds;
        this->receivedVotes = other.receivedVotes;

        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }
    bool IsRegistered() const { return (pubKey.IsFullyValid() && pubKey.GetKeyId() == keyID); }
    bool SetRegId(const CRegID &regID) {
        this->regID = regID;
        return true;
    };

    bool GetRegId(CRegID &regID) const {
        regID = this->regID;
        return !regID.IsEmpty();
    };

    uint64_t GetRawBalance();
    uint64_t GetTotalBalance();
    uint64_t GetFrozenBalance();
    uint64_t GetAccountProfit(uint64_t prevBlockHeight);
    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoinBalance > 0); }

    uint256 GetHash(bool recalculate = false) {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            //FIXME: need to check block soft-fork height here
            ss << regID << keyID << pubKey << minerPubKey
                << VARINT(bcoinBalance) << VARINT(scoinBalance) << VARINT(fcoinBalance)
                << VARINT(nVoteHeight)
                << vVoteFunds << receivedVotes;

            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    IMPLEMENT_SERIALIZE(
        READWRITE(regID);
        READWRITE(keyID);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(receivedVotes);)

    uint64_t GetReceiveVotes() const { return receivedVotes; }

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog {
public:
    CKeyID keyID;
    uint64_t bcoinBalance;         //!< freedom money which coinage greater than 30 days
    uint64_t nVoteHeight;          //!< account vote height
    vector<CVoteFund> vVoteFunds;  //!< delegate votes
    uint64_t receivedVotes;        //!< votes received

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(receivedVotes);)

public:
    CAccountLog(const CAccount &acct) {
        keyID        = acct.keyID;
        bcoinBalance = acct.bcoinBalance;
        nVoteHeight  = acct.nVoteHeight;
        vVoteFunds   = acct.vVoteFunds;
        receivedVotes= acct.receivedVotes;
    }
    CAccountLog(CKeyID &keyId) {
        keyID        = keyId;
        bcoinBalance = 0;
        nVoteHeight  = 0;
        receivedVotes= 0;
    }
    CAccountLog() {
        keyID        = uint160();
        bcoinBalance = 0;
        nVoteHeight  = 0;
        vVoteFunds.clear();
        receivedVotes = 0;
    }
    void SetValue(const CAccount &acct) {
        keyID        = acct.keyID;
        bcoinBalance = acct.bcoinBalance;
        nVoteHeight  = acct.nVoteHeight;
        receivedVotes= acct.receivedVotes;
        vVoteFunds   = acct.vVoteFunds;
    }
    string ToString() const;
};

#endif