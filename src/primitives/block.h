// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "primitives/cvn.h"
#include "serialize.h"
#include "uint256.h"

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    static const int32_t          CURRENT_VERSION = 1;
    static const int32_t               TX_PAYLOAD = 1 << 8;
    static const int32_t              CVN_PAYLOAD = 1 << 9;
    static const int32_t CHAIN_PARAMETERS_PAYLOAD = 1 << 10;
    static const int32_t     CHAIN_ADMINS_PAYLOAD = 1 << 11;
    static const int32_t      COIN_SUPPLY_PAYLOAD = 1 << 12;
    static const int32_t       ADMIN_PAYLOAD_MASK = CVN_PAYLOAD | CHAIN_PARAMETERS_PAYLOAD | CHAIN_ADMINS_PAYLOAD | COIN_SUPPLY_PAYLOAD;
    static const int32_t             PAYLOAD_MASK = TX_PAYLOAD | CVN_PAYLOAD | CHAIN_PARAMETERS_PAYLOAD | CHAIN_ADMINS_PAYLOAD | COIN_SUPPLY_PAYLOAD;
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nCreatorId;

    CSchnorrSig chainMultiSig;
    /* contains the CVN IDs of all the currently active CVN that failed
     * to sign the block, ideally empty
     */
    vector<uint32_t> vMissingCreatorIds;

    CSchnorrSig adminMultiSig;
    /* contains the admin IDs of all the currently active CVN that signed
     * the block
     */
    vector<uint32_t> vAdminIds;

    CBlockHeader()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nCreatorId);
        READWRITE(chainMultiSig);
        READWRITE(vMissingCreatorIds);
        if (HasAdminPayload()) {
            READWRITE(adminMultiSig);
            READWRITE(vAdminIds);
        }
    }

    void SetNull()
    {
        nVersion = CBlockHeader::CURRENT_VERSION;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nCreatorId = 0;
        chainMultiSig.SetNull();
        vMissingCreatorIds.clear();
        adminMultiSig.SetNull();
        vAdminIds.clear();
    }

    bool IsNull() const
    {
        return (nCreatorId == 0);
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    bool HasCvnInfo() const
    {
        return (nVersion & CVN_PAYLOAD);
    }

    bool HasChainParameters() const
    {
        return (nVersion & CHAIN_PARAMETERS_PAYLOAD);
    }

    bool HasTx() const
    {
        return (nVersion & TX_PAYLOAD);
    }

    bool HasChainAdmins() const
    {
        return (nVersion & CHAIN_ADMINS_PAYLOAD);
    }

    bool HasCoinSupplyPayload() const
    {
        return (nVersion & COIN_SUPPLY_PAYLOAD);
    }

    bool HasAdminPayload() const
    {
        return (nVersion & ADMIN_PAYLOAD_MASK);
    }

    uint256 GetHash() const;
    uint32_t GetNumChainSigs() const;

};

class CBlock : public CBlockHeader
{
public:
    // network and disk
    CSchnorrSig creatorSignature;
    std::vector<CTransaction> vtx;
    std::vector<CCvnInfo> vCvns;
    std::vector<CChainAdmin> vChainAdmins;
    CDynamicChainParams dynamicChainParams;
    CCoinSupply coinSupply;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(creatorSignature);

        if (HasTx())
            READWRITE(vtx);
        if (HasCvnInfo())
            READWRITE(vCvns);
        if (HasChainAdmins())
            READWRITE(vChainAdmins);
        if (HasChainParameters())
            READWRITE(dynamicChainParams);
        if (HasCoinSupplyPayload())
            READWRITE(coinSupply);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        vCvns.clear();
        creatorSignature.SetNull();
        vChainAdmins.clear();
        dynamicChainParams = CDynamicChainParams();
        coinSupply = CCoinSupply();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion           = nVersion;
        block.hashPrevBlock      = hashPrevBlock;
        block.hashMerkleRoot     = hashMerkleRoot;
        block.nTime              = nTime;
        block.nCreatorId         = nCreatorId;
        block.chainMultiSig      = chainMultiSig;
        block.vMissingCreatorIds = vMissingCreatorIds;
        block.adminMultiSig      = adminMultiSig;
        block.vAdminIds          = vAdminIds;
        return block;
    }

    std::string ToString() const;

    uint256 HashCVNs() const;

    uint256 HashChainAdmins() const;

    uint256 GetChainAdminDataHash() const;
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
