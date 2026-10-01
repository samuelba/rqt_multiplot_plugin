/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilterDropDialog.hpp"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHash>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveDataSequencer.hpp"
#include "rqt_multiplot/CurveFilterParamsWidget.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

const QString kCurveFilterMimeType = QStringLiteral("application/x-rqt-multiplot-curve-filter");

namespace {

constexpr int kCurveIndexRole = Qt::UserRole;

QHash<int, CurveFilterSpec>& lastUsedSpecs() {
  static QHash<int, CurveFilterSpec> specs;
  return specs;
}

}  // namespace

QMimeData* createCurveFilterMimeData(CurveFilterType type) {
  auto* mimeData = new QMimeData();
  mimeData->setData(kCurveFilterMimeType, curveFilterTypeKey(type).toUtf8());
  return mimeData;
}

std::optional<CurveFilterType> decodeCurveFilterMimeData(const QMimeData* mimeData) {
  if ((mimeData == nullptr) || !mimeData->hasFormat(kCurveFilterMimeType)) {
    return std::nullopt;
  }
  return curveFilterTypeFromKey(QString::fromUtf8(mimeData->data(kCurveFilterMimeType)));
}

CurveFilterDropDialog::CurveFilterDropDialog(QWidget* parent, CurveFilterType type, const PlotConfig& plot)
    : QDialog(parent),
      params_(new CurveFilterParamsWidget(this)),
      curveList_(new QListWidget(this)),
      buttonBox_(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this)) {
  setObjectName(QStringLiteral("curveFilterDropDialog"));
  setWindowTitle(tr("Add filter: %1").arg(curveFilterTypeName(type)));

  params_->setSpec(lastUsedSpec(type));
  auto* paramsGroup = new QGroupBox(tr("Parameters"), this);
  auto* paramsLayout = new QVBoxLayout(paramsGroup);
  paramsLayout->addWidget(params_);

  curveList_->setObjectName(QStringLiteral("curveFilterDropCurveList"));
  for (size_t index = 0; index < plot.getNumCurves(); ++index) {
    const CurveConfig* curveConfig = plot.getCurveConfig(index);
    if (curveConfig == nullptr) {
      continue;
    }
    auto* item = new QListWidgetItem(curveConfig->getTitle(), curveList_);
    item->setData(kCurveIndexRole, static_cast<int>(index));
    if (CurveDataSequencer::isSnapshotConfig(*curveConfig)) {
      item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable));
      item->setCheckState(Qt::Unchecked);
      item->setToolTip(tr("Filters do not apply to array snapshot curves"));
    } else {
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      item->setCheckState(Qt::Checked);
    }
  }
  auto* curvesGroup = new QGroupBox(tr("Curves"), this);
  auto* curvesLayout = new QVBoxLayout(curvesGroup);
  curvesLayout->addWidget(curveList_);

  buttonBox_->setObjectName(QStringLiteral("curveFilterDropButtonBox"));
  connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(curveList_, &QListWidget::itemChanged, this, [this]() { updateOkButton(); });

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(paramsGroup);
  layout->addWidget(curvesGroup, 1);
  layout->addWidget(buttonBox_);
  updateOkButton();
  Theme::apply(this);
}

CurveFilterDropDialog::~CurveFilterDropDialog() = default;

CurveFilterDropDialog::Result CurveFilterDropDialog::getResult() const {
  Result result{params_->getSpec(), {}};
  for (int row = 0; row < curveList_->count(); ++row) {
    const QListWidgetItem* item = curveList_->item(row);
    if (item->checkState() == Qt::Checked) {
      result.curveIndices.push_back(item->data(kCurveIndexRole).toInt());
    }
  }
  return result;
}

std::optional<CurveFilterDropDialog::Result> CurveFilterDropDialog::ask(QWidget* parent, CurveFilterType type, const PlotConfig& plot) {
  if (!curveFilterHasParameters(type)) {
    return Result{defaultCurveFilterSpec(type), filterableCurveIndices(plot)};
  }

  CurveFilterDropDialog dialog(parent, type, plot);
  if (dialog.exec() != QDialog::Accepted) {
    return std::nullopt;
  }
  const Result result = dialog.getResult();
  rememberSpec(result.spec);
  return result;
}

QVector<int> CurveFilterDropDialog::filterableCurveIndices(const PlotConfig& plot) {
  QVector<int> indices;
  for (size_t index = 0; index < plot.getNumCurves(); ++index) {
    const CurveConfig* curveConfig = plot.getCurveConfig(index);
    if ((curveConfig != nullptr) && !CurveDataSequencer::isSnapshotConfig(*curveConfig)) {
      indices.push_back(static_cast<int>(index));
    }
  }
  return indices;
}

CurveFilterSpec CurveFilterDropDialog::lastUsedSpec(CurveFilterType type) {
  return lastUsedSpecs().value(static_cast<int>(type), defaultCurveFilterSpec(type));
}

void CurveFilterDropDialog::rememberSpec(const CurveFilterSpec& spec) {
  lastUsedSpecs().insert(static_cast<int>(spec.type), spec);
}

void CurveFilterDropDialog::forgetSpecs() {
  lastUsedSpecs().clear();
}

void CurveFilterDropDialog::updateOkButton() {
  bool anyChecked = false;
  for (int row = 0; row < curveList_->count(); ++row) {
    anyChecked = anyChecked || (curveList_->item(row)->checkState() == Qt::Checked);
  }
  buttonBox_->button(QDialogButtonBox::Ok)->setEnabled(anyChecked);
}

}  // namespace rqt_multiplot
