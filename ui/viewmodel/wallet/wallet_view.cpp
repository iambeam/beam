// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "wallet_view.h"

#include <iomanip>

#include <QApplication>
#include <QtGui/qimage.h>
#include <QtCore/qbuffer.h>
#include <QUrlQuery>
#include <QClipboard>

#include "qrcode/QRCodeGenerator.h"
#include "utility/helpers.h"
#include "model/app_model.h"
#include "model/qr.h"
#include "viewmodel/ui_helpers.h"

using namespace beam;
using namespace beam::wallet;
using namespace std;
using namespace beamui;

namespace
{
    template<typename T>
    bool compareTx(const T& lf, const T& rt, Qt::SortOrder sortOrder)
    {
        if (sortOrder == Qt::DescendingOrder)
            return lf > rt;
        return lf < rt;
    }
}

WalletViewModel::WalletViewModel()
    : _model(*AppModel::getInstance().getWallet())
    , _settings(AppModel::getInstance().getSettings())
{
    connect(&_model, SIGNAL(txStatus(beam::wallet::ChangeAction, const std::vector<beam::wallet::TxDescription>&)),
        SLOT(onTxStatus(beam::wallet::ChangeAction, const std::vector<beam::wallet::TxDescription>&)));

    connect(&_model, SIGNAL(addressesChanged(bool, const std::vector<beam::wallet::WalletAddress>&)),
        SLOT(onAddresses(bool, const std::vector<beam::wallet::WalletAddress>&)));

    _status.setOnChanged([this]() {
        emit stateChanged();
    });

    // TODO: This also refreshes TXs and addresses. Need to make this more transparent
    _status.refresh();
}

WalletViewModel::~WalletViewModel()
{
    qDeleteAll(_txList);
}

void WalletViewModel::cancelTx(TxObject* pTxObject)
{
    if (pTxObject->canCancel())
    {
        _model.getAsync()->cancelTx(pTxObject->getTxDescription().m_txId);
    }
}

void WalletViewModel::deleteTx(TxObject* pTxObject)
{
    if (pTxObject->canDelete())
    {
        _model.getAsync()->deleteTx(pTxObject->getTxDescription().m_txId);
    }
}

void WalletViewModel::onTxStatus(beam::wallet::ChangeAction action, const std::vector<beam::wallet::TxDescription>& items)
{
    QList<TxObject*> deletedObjects;
    if (action == beam::wallet::ChangeAction::Reset)
    {
        deletedObjects.swap(_txList);
        _txList.clear();
        for (const auto& item : items)
        {
            _txList.push_back(new TxObject(item));
        }
        sortTx();
    }
    else if (action == beam::wallet::ChangeAction::Removed)
    {
        for (const auto& item : items)
        {
            auto it = find_if(_txList.begin(), _txList.end(), [&item](const auto& tx) {return item.m_txId == tx->getTxDescription().m_txId; });
            if (it != _txList.end())
            {
                deletedObjects.push_back(*it);
                _txList.erase(it);
            }
        }
        emit transactionsChanged();
    }
    else if (action == beam::wallet::ChangeAction::Updated)
    {
        auto txIt = _txList.begin();
        auto txEnd = _txList.end();
        for (const auto& item : items)
        {
            txIt = find_if(txIt, txEnd, [&item](const auto& tx) {return item.m_txId == tx->getTxDescription().m_txId; });
            if (txIt == txEnd)
            {
                // insert new object
                _txList.insert(0, new TxObject(item));
                continue;
            }
            (*txIt)->update(item);
        }
        sortTx();
    }
    else if (action == beam::wallet::ChangeAction::Added)
    {
        // TODO in sort order
        for (const auto& item : items)
        {
            _txList.insert(0, new TxObject(item));
        }
        sortTx();
    }

    qDeleteAll(deletedObjects);

    // Get info for TxObject::_user_name (get wallets labels)
    _model.getAsync()->getAddresses(false);

}

double WalletViewModel::beamAvailable() const
{
    return double(int64_t(_status.getAvailable())) / Rules::Coin;
}

double WalletViewModel::beamReceiving() const
{
    // TODO:SWAP return real value
    return beamReceivingChange() + beamReceivingIncoming();
}

double WalletViewModel::beamSending() const
{
    return double(_status.getSending()) / Rules::Coin;
}

double WalletViewModel::beamReceivingChange() const
{
    // TODO:SWAP return real value
    return double(_status.getReceivingChange()) / Rules::Coin;
}

double WalletViewModel::beamReceivingIncoming() const
{
    // TODO:SWAP return real value
    return double(_status.getReceivingIncoming()) / Rules::Coin;
}

double WalletViewModel::beamLocked() const
{
    return beamLockedMaturing();
}

double WalletViewModel::beamLockedMaturing() const
{
    return double(_status.getMaturing()) / Rules::Coin;
}

QString WalletViewModel::sortRole() const
{
    return _sortRole;
}

void WalletViewModel::setSortRole(const QString& value)
{
    if (value != getDateRole() && value != getAmountRole() && value != getSentAmountRole() && value != getReceivedAmountRole() &&
        value != getStatusRole() && value != getSendingAddressRole() && value != getReceivingAddressRole())
        return;

    _sortRole = value;
    sortTx();
}

Qt::SortOrder WalletViewModel::sortOrder() const
{
    return _sortOrder;
}

void WalletViewModel::setSortOrder(Qt::SortOrder value)
{
    _sortOrder = value;
    sortTx();
}

QString WalletViewModel::getIncomeRole() const
{
    return "income";
}

QString WalletViewModel::getDateRole() const
{
    return "date";
}

QString WalletViewModel::getDisplayNameRole() const
{
    return "displayName";
}

QString WalletViewModel::getSendingAddressRole() const
{
    return "sendingAddress";
}

QString WalletViewModel::getReceivingAddressRole() const
{
    return "receivingAddress";
}

QString WalletViewModel::getAmountRole() const
{
    return "amount";
}

QString WalletViewModel::getSentAmountRole() const
{
    return "sentAmount";
}

QString WalletViewModel::getReceivedAmountRole() const
{
    return "receivedAmount";
}

QString WalletViewModel::getStatusRole() const
{
    return "status";
}

bool WalletViewModel::isAllowedBeamMWLinks() const
{
    return _settings.isAllowedBeamMWLinks();
}

void WalletViewModel::allowBeamMWLinks(bool value)
{
    _settings.setAllowedBeamMWLinks(value);
}

QQmlListProperty<TxObject> WalletViewModel::getTransactions()
{
    return QQmlListProperty<TxObject>(this, _txList);
}

void WalletViewModel::sortTx()
{
    auto cmp = generateComparer();
    std::sort(_txList.begin(), _txList.end(), cmp);

    emit transactionsChanged();
}

std::function<bool(const TxObject*, const TxObject*)> WalletViewModel::generateComparer()
{
    if (_sortRole == getIncomeRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getTxDescription().m_sender, rt->getTxDescription().m_sender, sortOrder);
    };

    if (_sortRole == getSendingAddressRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getSendingAddress(), rt->getSendingAddress(), sortOrder);
    };

    if (_sortRole == getReceivingAddressRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getReceivingAddress(), rt->getReceivingAddress(), sortOrder);
    };

    if (_sortRole == getDisplayNameRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->displayName(), rt->displayName(), sortOrder);
    };

    if (_sortRole == getAmountRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getAmountValue(), rt->getAmountValue(), sortOrder);
    };


    if (_sortRole == getSentAmountRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getSentAmountValue(), rt->getSentAmountValue(), sortOrder);
    };

    if (_sortRole == getReceivedAmountRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getReceivedAmountValue(), rt->getReceivedAmountValue(), sortOrder);
    };

    if (_sortRole == getStatusRole())
        return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->status(), rt->status(), sortOrder);
    };

    // defult for dateRole
    return [sortOrder = _sortOrder](const TxObject* lf, const TxObject* rt)
    {
        return compareTx(lf->getTxDescription().m_createTime, rt->getTxDescription().m_createTime, sortOrder);
    };
}

void WalletViewModel::onAddresses(bool own, const std::vector<beam::wallet::WalletAddress>& addresses)
{
    if (own)
    {
        return;
    }

    for (auto* tx : _txList)
    {
        auto foundIter = std::find_if(addresses.cbegin(), addresses.cend(),
                                      [tx](const auto& address) { return address.m_walletID == tx->peerId(); });

        if (foundIter != addresses.cend())
        {
            tx->setUserName(QString::fromStdString(foundIter->m_label));
        }
        else if (!tx->userName().isEmpty())
        {
            tx->setUserName(QString{});
        }

        auto displayName = tx->userName();// .isEmpty() ? tx->user() : tx->userName();
        tx->setDisplayName(displayName);
    }
}
