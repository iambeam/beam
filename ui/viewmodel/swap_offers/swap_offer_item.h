// Copyright 2019 The Beam Team
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

#pragma once

#include <QObject>
#include <QDateTime>
#include "model/wallet_model.h"
#include "viewmodel/ui_helpers.h"

using namespace beam::wallet;

class SwapOfferItem : public QObject
{
    Q_OBJECT
    // Q_PROPERTY(QDateTime    timeCreated         READ timeCreated        NOTIFY changed)
    // Q_PROPERTY(QDateTime    timeExpiration      READ timeExpiration     NOTIFY changed)
    // Q_PROPERTY(QString      amountSend          READ amountSend         NOTIFY changed)
    // Q_PROPERTY(QString      amountReceive       READ amountReceive      NOTIFY changed)
    // Q_PROPERTY(double       rate                READ rate               NOTIFY changed)
    // Q_PROPERTY(bool         isOwnOffer          READ isOwnOffer         NOTIFY changed)
    // Q_PROPERTY(TxParameters txParameters        READ getTxParameters    NOTIFY changed)

public:
    SwapOfferItem() = default;
    SwapOfferItem(const SwapOffer& offer, bool isOwn, const QDateTime& timeExpiration);

    auto timeCreated() const -> QDateTime;
    auto timeExpiration() const -> QDateTime;
    auto amountSend() const -> QString;
    auto amountReceive() const -> QString;
    auto rate() const -> double;
    auto isOwnOffer() const -> bool;

    auto rawAmountSend() const -> beam::Amount;
    auto rawAmountReceive() const -> beam::Amount;

    auto getTxParameters() const -> TxParameters;
    auto getTxID() const -> TxID;

signals:
    // void changed();

private:
    auto getSwapCoinType() const -> beamui::Currencies;

    SwapOffer m_offer;          /// raw TxParameters
    bool m_isOwnOffer;          /// indicates if offer belongs to this wallet
    bool m_isBeamSide;          /// pay beam to receive other
    QDateTime m_timeExpiration;
};
