// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRICE_FEED_H
#define PRICE_FEED_H

#include "tx/tx.h"

class CPricePoint {
private:
    unsigned char coinType;
    unsigned char priceType;
    uint64_t price;

public:
    CPricePoint(CoinType coinTypeIn, PriceType priceTypeIn, uint64_t priceIn)
        : coinType(coinTypeIn), priceType(priceTypeIn), price(priceIn) {}

    string ToString() {
        return strprintf("coinType:%u, priceType:%u, price:%lld", coinType, priceType, price);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinType);
        READWRITE(priceType);
        READWRITE(VARINT(price));)
};

class CPriceFeedTx : public CBaseTx {
private:
    vector<CPricePoint> pricePoints;

public:
    CPriceFeedTx(): CBaseTx(PRICE_FEED_TX) {}
    CPriceFeedTx(const CBaseTx *pBaseTx): CBaseTx(PRICE_FEED_TX) {
        assert(PRICE_FEED_TX == pBaseTx->nTxType);
        *this = *(CPriceFeedTx *)pBaseTx;
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                const CPricePoint &pricePointIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeIn) {
        pricePoints.push_back(pricePointIn);
    }
    CPriceFeedTx(const CUserID &txUidIn, int validHeightIn, uint64_t feeIn,
                const vector<CPricePoint> &pricePointsIn):
        CBaseTx(PRICE_FEED_TX, txUidIn, validHeightIn, feeIn) {
        if (pricePoints.size() > 3 || pricePoints.size() == 0)
            return; // limit max # of price points to be three in one shot

        pricePoints = pricePointsIn;
    }

    ~CPriceFeedTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        for(auto const& pricePoint: pricePoints) {
            READWRITE(pricePoint);
        };)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << pricePoints;
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceFeedTx>(this); }
    virtual double GetPriority() const { return 10000.0f; } // Top priority

    virtual bool CheckTx(CValidationState &state, CAccountCache &view, CContractCache &scriptDB);
    virtual bool ExecuteTx(int nIndex, CAccountCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionCache &txCache, CContractCache &scriptDB);
    virtual bool UndoExecuteTx(int nIndex, CAccountCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionCache &txCache,
                    CContractCache &scriptDB);
    virtual string ToString(CAccountCache &view) const; //logging usage
    virtual Object ToJson(const CAccountCache &view) const; //json-rpc usage
    virtual bool GetInvolvedKeyIds(set<CKeyID> &vAddr, CAccountCache &view, CContractCache &scriptDB);

};

class CBlockPriceMedianTx: public CBaseTx  {
private:
    map<tuple<CoinType, PriceType>, uint64_t> mapMediaPricePoints;

public:
    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        for (auto it = mapMediaPricePoints.begin(); it! = mapMediaPricePoints.end(); ++it) {
            unsigned char coinType  = it->first()->first();
            unsigned char priceType = it->first()->second();
            uint64_t price = it->second();
            READWRITE(PricePoint(coinType, priceType, price));
        };
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nHeight) << txUid
                << mapMediaPricePoints;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint64_t GetValue() const { return rewardValue; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CPriceMedianTx>(this); }
    virtual uint64_t GetFee() const { return 0; }
    virtual double GetPriority() const { return 0.0f; }
    virtual string ToString(const CAccountCache &view) const;
    virtual Object ToJson(const CAccountCache &view) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);

    inline uint64_t GetMedianPriceByType(const CoinType coinType, const PriceType priceType);
};

#endif