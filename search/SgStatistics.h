
#ifndef SG_STATISTICS_H
#define SG_STATISTICS_H

#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "platform/SgException.h"
#include "board/SgWrite.h"

template<typename VALUE, typename COUNT>
class UctStatisticsBase {
 public:
  UctStatisticsBase();
  UctStatisticsBase(VALUE val, COUNT count);
  void Add(VALUE val);
  void Remove(VALUE val);
  void Add(VALUE val, COUNT n);
  void Remove(VALUE val, COUNT n);
  void Clear();
  COUNT Count() const;
  void Initialize(VALUE val, COUNT count);
  bool IsDefined() const;
  VALUE Mean() const;

  void Write(std::ostream &out) const;
  void SaveAsText(std::ostream &out) const;
  void LoadFromText(std::istream &in);

 private:
  COUNT m_count;
  VALUE m_mean;
};

template<typename VALUE, typename COUNT>
inline UctStatisticsBase<VALUE, COUNT>::UctStatisticsBase() {
  Clear();
}

template<typename VALUE, typename COUNT>
inline UctStatisticsBase<VALUE, COUNT>::UctStatisticsBase(VALUE val, COUNT count)
    : m_count(count),
      m_mean(val) {}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::Add(VALUE val) {
  // Write order dependency: at least one class (SgUctSearch in lock-free
  // mode) uses UctStatisticsBase concurrently without locking and assumes
  // that m_mean is valid, if m_count is greater zero
  COUNT count = m_count;
  ++count;
  DBG_ASSERT(!std::numeric_limits<COUNT>::is_exact
                || count > 0); // overflow
  val -= m_mean;
  m_mean += val / VALUE(count);
  m_count = count;
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::Remove(VALUE val) {
  // Write order dependency: at least on class (SgUctSearch in lock-free
  // mode) uses UctStatisticsBase concurrently without locking and assumes
  // that m_mean is valid, if m_count is greater zero
  COUNT count = m_count;
  if (count > 1) {
    --count;
    m_mean += (m_mean - val) / VALUE(count);
    m_count = count;
  } else
    Clear();
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::Remove(VALUE val, COUNT n) {
  DBG_ASSERT(m_count >= n);
  // Write order dependency: at least on class (SgUctSearch in lock-free
  // mode) uses UctStatisticsBase concurrently without locking and assumes
  // that m_mean is valid, if m_count is greater zero
  COUNT count = m_count;
  if (count > n) {
    count -= n;
    m_mean += VALUE(n) * (m_mean - val) / VALUE(count);
    m_count = count;
  } else
    Clear();
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::Add(VALUE val, COUNT n) {
  // Write order dependency: at least one class (SgUctSearch in lock-free
  // mode) uses UctStatisticsBase concurrently without locking and assumes
  // that m_mean is valid, if m_count is greater zero
  COUNT count = m_count;
  count += n;
  DBG_ASSERT(!std::numeric_limits<COUNT>::is_exact
                || count > 0); // overflow
  val -= m_mean;
  m_mean += VALUE(n) * val / VALUE(count);
  m_count = count;
}

template<typename VALUE, typename COUNT>
inline void UctStatisticsBase<VALUE, COUNT>::Clear() {
  m_count = 0;
  m_mean = 0;
}

template<typename VALUE, typename COUNT>
inline COUNT UctStatisticsBase<VALUE, COUNT>::Count() const {
  return m_count;
}

template<typename VALUE, typename COUNT>
inline void UctStatisticsBase<VALUE, COUNT>::Initialize(VALUE val, COUNT count) {
  DBG_ASSERT(count > 0);
  m_count = count;
  m_mean = val;
}

template<typename VALUE, typename COUNT>
inline bool UctStatisticsBase<VALUE, COUNT>::IsDefined() const {
  if (std::numeric_limits<COUNT>::is_exact)
    return m_count > 0;
  else
    return m_count > std::numeric_limits<COUNT>::epsilon();
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::LoadFromText(std::istream &in) {
  in >> m_count >> m_mean;
}

template<typename VALUE, typename COUNT>
inline VALUE UctStatisticsBase<VALUE, COUNT>::Mean() const {
  DBG_ASSERT(IsDefined());
  return m_mean;
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::Write(std::ostream &out) const {
  if (IsDefined())
    out << Mean();
  else
    out << '-';
}

template<typename VALUE, typename COUNT>
void UctStatisticsBase<VALUE, COUNT>::SaveAsText(std::ostream &out) const {
  out << m_count << ' ' << m_mean;
}


template<typename VALUE, typename COUNT>
class SgStatistics {
 public:
  SgStatistics();
  SgStatistics(VALUE val, COUNT count);
  void Add(VALUE val);
  void Clear();
  bool IsDefined() const;
  VALUE Mean() const;
  COUNT Count() const;
  VALUE Deviation() const;
  VALUE Variance() const;
  void Write(std::ostream &out) const;
  void SaveAsText(std::ostream &out) const;
  void LoadFromText(std::istream &in);

 private:
  UctStatisticsBase<VALUE, COUNT> m_statisticsBase;
  VALUE m_variance;
};

template<typename VALUE, typename COUNT>
inline SgStatistics<VALUE, COUNT>::SgStatistics() {
  Clear();
}

template<typename VALUE, typename COUNT>
inline SgStatistics<VALUE, COUNT>::SgStatistics(VALUE val, COUNT count)
    : m_statisticsBase(val, count) {
  m_variance = 0;
}

template<typename VALUE, typename COUNT>
void SgStatistics<VALUE, COUNT>::Add(VALUE val) {
  if (IsDefined()) {
    COUNT countOld = Count();
    VALUE meanOld = Mean();
    m_statisticsBase.Add(val);
    VALUE mean = Mean();
    COUNT count = Count();
    m_variance = (VALUE(countOld) * (m_variance + meanOld * meanOld)
        + val * val) / VALUE(count) - mean * mean;
  } else {
    m_statisticsBase.Add(val);
    m_variance = 0;
  }
}

template<typename VALUE, typename COUNT>
inline void SgStatistics<VALUE, COUNT>::Clear() {
  m_statisticsBase.Clear();
  m_variance = 0;
}

template<typename VALUE, typename COUNT>
inline COUNT SgStatistics<VALUE, COUNT>::Count() const {
  return m_statisticsBase.Count();
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatistics<VALUE, COUNT>::Deviation() const {
  return (VALUE) std::sqrt(m_variance);
}

template<typename VALUE, typename COUNT>
inline bool SgStatistics<VALUE, COUNT>::IsDefined() const {
  return m_statisticsBase.IsDefined();
}

template<typename VALUE, typename COUNT>
void SgStatistics<VALUE, COUNT>::LoadFromText(std::istream &in) {
  m_statisticsBase.LoadFromText(in);
  in >> m_variance;
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatistics<VALUE, COUNT>::Mean() const {
  return m_statisticsBase.Mean();
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatistics<VALUE, COUNT>::Variance() const {
  return m_variance;
}

template<typename VALUE, typename COUNT>
void SgStatistics<VALUE, COUNT>::Write(std::ostream &out) const {
  if (IsDefined())
    out << Mean() << " dev=" << Deviation();
  else
    out << '-';
}

template<typename VALUE, typename COUNT>
void SgStatistics<VALUE, COUNT>::SaveAsText(std::ostream &out) const {
  m_statisticsBase.SaveAsText(out);
  out << ' ' << m_variance;
}


template<typename VALUE, typename COUNT>
class SgStatisticsExt {
 public:
  SgStatisticsExt();
  void Add(VALUE val);
  void Clear();
  bool IsDefined() const;
  VALUE Mean() const;
  COUNT Count() const;
  VALUE Max() const;
  VALUE Min() const;
  VALUE Deviation() const;
  VALUE Variance() const;
  void Write(std::ostream &out) const;

 private:
  SgStatistics<VALUE, COUNT> m_statistics;
  VALUE m_max;
  VALUE m_min;
};

template<typename VALUE, typename COUNT>
inline SgStatisticsExt<VALUE, COUNT>::SgStatisticsExt() {
  Clear();
}

template<typename VALUE, typename COUNT>
void SgStatisticsExt<VALUE, COUNT>::Add(VALUE val) {
  m_statistics.Add(val);
  if (val > m_max)
    m_max = val;
  if (val < m_min)
    m_min = val;
}

template<typename VALUE, typename COUNT>
inline void SgStatisticsExt<VALUE, COUNT>::Clear() {
  m_statistics.Clear();
  m_min = std::numeric_limits<VALUE>::max();
  m_max = -std::numeric_limits<VALUE>::max();
}

template<typename VALUE, typename COUNT>
inline COUNT SgStatisticsExt<VALUE, COUNT>::Count() const {
  return m_statistics.Count();
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatisticsExt<VALUE, COUNT>::Deviation() const {
  return m_statistics.Deviation();
}

template<typename VALUE, typename COUNT>
inline bool SgStatisticsExt<VALUE, COUNT>::IsDefined() const {
  return m_statistics.IsDefined();
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatisticsExt<VALUE, COUNT>::Max() const {
  return m_max;
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatisticsExt<VALUE, COUNT>::Mean() const {
  return m_statistics.Mean();
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatisticsExt<VALUE, COUNT>::Min() const {
  return m_min;
}

template<typename VALUE, typename COUNT>
inline VALUE SgStatisticsExt<VALUE, COUNT>::Variance() const {
  return m_statistics.Variance();
}

template<typename VALUE, typename COUNT>
void SgStatisticsExt<VALUE, COUNT>::Write(std::ostream &out) const {
  if (IsDefined()) {
    m_statistics.Write(out);
    out << " min=" << m_min << " max=" << m_max;
  } else
    out << '-';
}


template<typename VALUE, typename COUNT>
class SgStatisticsCollection {
 public:
  void Add(const SgStatisticsCollection<VALUE, COUNT> &collection);
  void Clear();
  bool Contains(const std::string &name) const;
  void Create(const std::string &name);
  const SgStatistics<VALUE, COUNT> &Get(const std::string &name) const;
  SgStatistics<VALUE, COUNT> &Get(const std::string &name);
  void Write(std::ostream &o) const;

 private:
  typedef std::map<std::string, SgStatistics<VALUE, COUNT> > Map;
  typedef typename Map::iterator Iterator;
  typedef typename Map::const_iterator ConstIterator;
  Map m_map;
};

template<typename VALUE, typename COUNT>
void
SgStatisticsCollection<VALUE, COUNT>
::Add(const SgStatisticsCollection<VALUE, COUNT> &collection) {
  if (m_map.size() != collection.m_map.size())
    throw SgException("Incompatible statistics collections");
  for (Iterator p = m_map.begin(); p != m_map.end(); ++p) {
    ConstIterator k = collection.m_map.find(p->first);
    if (k == collection.m_map.end())
      throw SgException("Incompatible statistics collections");
    p->second.Add(k->second);
  }
}

template<typename VALUE, typename COUNT>
void SgStatisticsCollection<VALUE, COUNT>::Clear() {
  for (Iterator p = m_map.begin(); p != m_map.end(); ++p)
    p->second.Clear();
}

template<typename VALUE, typename COUNT>
bool SgStatisticsCollection<VALUE, COUNT>::Contains(const std::string &name)
const {
  return (m_map.find(name) != m_map.end());
}

template<typename VALUE, typename COUNT>
void SgStatisticsCollection<VALUE, COUNT>::Create(const std::string &name) {
  m_map[name] = SgStatistics<VALUE, COUNT>();
}

template<typename VALUE, typename COUNT>
const SgStatistics<VALUE, COUNT> &
SgStatisticsCollection<VALUE, COUNT>::Get(const std::string &name) const {
  ConstIterator p = m_map.find(name);
  if (p == m_map.end()) {
    std::ostringstream o;
    o << "Unknown statistics name " << name << '.';
    throw SgException(o.str());
  }
  return p->second;
}

template<typename VALUE, typename COUNT>
SgStatistics<VALUE, COUNT> &
SgStatisticsCollection<VALUE, COUNT>::Get(const std::string &name) {
  Iterator p = m_map.find(name);
  if (p == m_map.end()) {
    std::ostringstream o;
    o << "Unknown statistics name " << name << '.';
    throw SgException(o.str());
  }
  return p->second;
}

template<typename VALUE, typename COUNT>
void SgStatisticsCollection<VALUE, COUNT>::Write(std::ostream &o) const {
  for (ConstIterator p = m_map.begin(); p != m_map.end(); ++p)
    o << p->first << ": " << p->second.Write(o) << '\n';
}


template<typename VALUE, typename COUNT>
class SgHistogram {
 public:
  SgHistogram();
  SgHistogram(VALUE min, VALUE max, int bins);
  void Init(VALUE min, VALUE max, int bins);
  void Add(VALUE value);
  void Clear();
  int Bins() const;
  COUNT Count() const;
  COUNT Count(int i) const;
  void Write(std::ostream &out) const;
  void WriteWithLabels(std::ostream &out, const std::string &label) const;

 private:
  typedef std::vector<COUNT> Vector;
  int m_bins;
  COUNT m_count;
  VALUE m_binSize;
  VALUE m_min;
  VALUE m_max;
  Vector m_array;
};

template<typename VALUE, typename COUNT>
SgHistogram<VALUE, COUNT>::SgHistogram() {
  Init(0, 1, 1);
}

template<typename VALUE, typename COUNT>
SgHistogram<VALUE, COUNT>::SgHistogram(VALUE min, VALUE max, int bins) {
  Init(min, max, bins);
}

template<typename VALUE, typename COUNT>
void SgHistogram<VALUE, COUNT>::Add(VALUE value) {
  ++m_count;
  int i = static_cast<int>((value - m_min) / m_binSize);
  if (i < 0)
    i = 0;
  if (i >= m_bins)
    i = m_bins - 1;
  ++m_array[i];
}

template<typename VALUE, typename COUNT>
int SgHistogram<VALUE, COUNT>::Bins() const {
  return m_bins;
}

template<typename VALUE, typename COUNT>
void SgHistogram<VALUE, COUNT>::Clear() {
  m_count = 0;
  for (typename Vector::iterator it = m_array.begin(); it != m_array.end();
       ++it)
    *it = 0;
}

template<typename VALUE, typename COUNT>
COUNT SgHistogram<VALUE, COUNT>::Count() const {
  return m_count;
}

template<typename VALUE, typename COUNT>
COUNT SgHistogram<VALUE, COUNT>::Count(int i) const {
  DBG_ASSERT(i >= 0);
  DBG_ASSERT(i < m_bins);
  return m_array[i];
}

template<typename VALUE, typename COUNT>
void SgHistogram<VALUE, COUNT>::Init(VALUE min, VALUE max, int bins) {
  m_array.resize(bins);
  m_min = min;
  m_max = max;
  m_bins = bins;
  m_binSize = (m_max - m_min) / m_bins;
  Clear();
}

template<typename VALUE, typename COUNT>
void SgHistogram<VALUE, COUNT>::Write(std::ostream &out) const {
  for (int i = 0; i < m_bins; ++i)
    out << (m_min + i * m_binSize) << '\t' << m_array[i] << '\n';

}

template<typename VALUE, typename COUNT>
void SgHistogram<VALUE, COUNT>::WriteWithLabels(std::ostream &out,
                                                const std::string &label) const {
  for (int i = 0; i < m_bins; ++i) {
    std::ostringstream binLabel;
    binLabel << label << '[' << (m_min + i * m_binSize) << ']';
    out << SgWriteLabel(binLabel.str()) << m_array[i] << '\n';
  }
}

#endif // SG_STATISTICS_H
