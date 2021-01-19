#include "plotwrapper.hpp"
#include "logger.hpp"
#include "symbolic.hpp"
#include <QColor>
#include <QFont>
#include <QString>

PlotWrapper::PlotWrapper(const QString &plotTitle, QWidget *parent)
    : plot(new QCustomPlot(parent)) {
  QFont font;
  if (parent != nullptr) {
    font = QFont(parent->font().family(), parent->font().pointSize() + 4,
                 QFont::Bold);
  }
  title = new QCPTextElement(plot, plotTitle, font);
  plot->setInteraction(QCP::iRangeDrag, true);
  plot->setInteraction(QCP::iRangeZoom, true);
  plot->setInteraction(QCP::iSelectPlottables, false);
  plot->legend->setVisible(true);
  verticalLine = new QCPItemStraightLine(plot);
  verticalLine->setVisible(false);
  plot->setObjectName(QString::fromUtf8("plot"));
  plot->plotLayout()->insertRow(0);
  plot->plotLayout()->addElement(0, 0, title);
}

void PlotWrapper::addAvMinMaxLine(const QString &name, QColor col) {
  SPDLOG_DEBUG("Adding line '{}', colour {:x}", name.toStdString(), col.rgb());
  // avg
  auto *av = plot->addGraph();
  av->setPen(col);
  av->setName(QString("%1 (average)").arg(name));
  av->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
  // min
  auto *min = plot->addGraph();
  col.setAlpha(30);
  min->setPen(col);
  min->setBrush(QBrush(col));
  min->setName(QString("%1 (min/max range)").arg(name));
  // max
  auto *max = plot->addGraph();
  max->setPen(col);
  min->setChannelFillGraph(max);
  plot->legend->removeItem(plot->legend->itemCount() - 1);
  species.push_back(name.toStdString());
  concs.emplace_back();
}

void PlotWrapper::addAvMinMaxPoint(int lineIndex, double time,
                                   const sme::simulate::AvgMinMax &concentration) {
  plot->graph(3 * lineIndex)->addData({time}, {concentration.avg}, true);
  plot->graph(3 * lineIndex + 1)->addData({time}, {concentration.min}, true);
  plot->graph(3 * lineIndex + 2)->addData({time}, {concentration.max}, true);
  concs[static_cast<std::size_t>(lineIndex)].push_back(concentration);
  if (plot->graph(0)->dataCount() > times.size()) {
    times.push_back(time);
  }
}

void PlotWrapper::addObservableLine(
    const PlotWrapperObservable &plotWrapperObservable, QColor col) {
  SPDLOG_DEBUG("Adding observable '{}' = '{}'",
               plotWrapperObservable.name.toStdString(),
               plotWrapperObservable.expression.toStdString());
  if (plot->graphCount() == 0) {
    return;
  }
  auto vars = species;
  vars.push_back("t");
  sme::utils::Symbolic sym(plotWrapperObservable.expression.toStdString(), vars);
  if (!sym.isValid()) {
    SPDLOG_WARN("Skipping invalid expression: '{}'",
                plotWrapperObservable.expression.toStdString());
    return;
  }
  auto *p = plot->addGraph();
  p->setPen(col);
  p->setName(plotWrapperObservable.name);
  p->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ScatterShape::ssDisc));
  QVector<double> obs;
  obs.reserve(static_cast<int>(concs.size()));
  double res;
  std::vector<double> c(concs.size() + 1, 0.0);
  for (std::size_t it = 0; it < static_cast<std::size_t>(times.size()); ++it) {
    for (std::size_t ic = 0; ic < species.size(); ++ic) {
      c[ic] = concs[ic][it].avg;
    }
    c.back() = times[static_cast<int>(it)];
    sym.eval(&res, c.data());
    obs.push_back(res);
  }
  p->setData(times, obs, true);
  observables.push_back(plotWrapperObservable);
}

void PlotWrapper::clearObservableLines() {
  SPDLOG_TRACE("Clearing {} observables", observables.size());
  int nSpecies{static_cast<int>(species.size())};
  int nObservables{static_cast<int>(observables.size())};
  for (int i = 3 * nSpecies + nObservables - 1; i >= 3 * nSpecies; --i) {
    if (plot->removeGraph(i)) {
      SPDLOG_TRACE("Removed graph {}", i);
    } else {
      SPDLOG_WARN("Failed to remove graph {}", i);
    }
  }
  observables.clear();
}

void PlotWrapper::setVerticalLine(double x) {
  verticalLine->setVisible(true);
  verticalLine->point1->setCoords(x, 0);
  verticalLine->point2->setCoords(x, 1);
  if (!plot->xAxis->range().contains(x)) {
    plot->rescaleAxes(true);
  }
}

void PlotWrapper::update(const std::vector<bool> &speciesVisible,
                         bool showMinMax) {
  int iSpecies = 0;
  plot->legend->clearItems();
  for (bool visible : speciesVisible) {
    bool minMaxVisible = visible && showMinMax;
    plot->graph(3 * iSpecies)->setVisible(visible);
    if (visible) {
      plot->graph(3 * iSpecies)->addToLegend();
    }
    plot->graph(3 * iSpecies + 1)->setVisible(minMaxVisible);
    plot->graph(3 * iSpecies + 2)->setVisible(minMaxVisible);
    if (minMaxVisible) {
      plot->graph(3 * iSpecies + 1)->addToLegend();
    }
    ++iSpecies;
  }
  int iObs = 0;
  for (const auto &observable : observables) {
    auto *g = plot->graph(3 * static_cast<int>(species.size()) + iObs);
    g->setVisible(observable.visible);
    if (observable.visible) {
      g->addToLegend();
    }
    ++iObs;
  }
  plot->rescaleAxes(true);
  plot->replot();
}

void PlotWrapper::clear() {
  plot->clearGraphs();
  plot->replot();
  observables.clear();
  species.clear();
  concs.clear();
  times.clear();
}

double PlotWrapper::xValue(const QMouseEvent *event) const {
  double x{static_cast<double>(event->x())};
  double y{static_cast<double>(event->y())};
  double key;
  double val;
  plot->graph(0)->pixelsToCoords(x, y, key, val);
  return key;
}

const QVector<double> &PlotWrapper::getTimepoints() const { return times; }

static QString makeCSVHeader(const std::vector<std::string> &species) {
  QString csv;
  if (species.empty()) {
    return {};
  }
  std::string header("time, ");
  for (const auto &spec : species) {
    header.append(fmt::format("{0} (avg), {0} (min), {0} (max), ", spec));
  }
  csv.append(header.c_str());
  csv.chop(2);
  csv.append("\n");
  return csv;
}

QString PlotWrapper::getDataAsCSV() const {
  auto csv{makeCSVHeader(species)};
  for (std::size_t it = 0; it < static_cast<std::size_t>(times.size()); ++it) {
    double t = times[static_cast<int>(it)];
    std::string row{fmt::format("{:.14e}, ", t)};
    for (std::size_t ic = 0; ic < species.size(); ++ic) {
      const auto &c = concs[ic][it];
      row.append(
          fmt::format("{:.14e}, {:.14e}, {:.14e}, ", c.avg, c.min, c.max));
    }
    csv.append(row.c_str());
    csv.chop(2);
    csv.append("\n");
  }
  return csv;
}
