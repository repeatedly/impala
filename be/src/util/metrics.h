// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_UTIL_METRICS_H
#define IMPALA_UTIL_METRICS_H

#include <map>
#include <string>
#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>

#include "common/logging.h"
#include "common/status.h"
#include "common/object-pool.h"

namespace impala {

class Webserver;

// Publishes execution metrics to a webserver page
// TODO: Reconsider naming here; Metrics is too general.
class Metrics {
 private:
  // Superclass for metric types, to allow for a single container to hold all metrics
  class GenericMetric {
   public:
    // Print key and value to a string
    virtual void Print(std::stringstream* out) = 0;

    // Print key and value in Json format
    virtual void PrintJson(std::stringstream* out) = 0;
  };

 public:
  // Structure containing a metric value. Provides for thread-safe update and 
  // test-and-set operations.
  template<typename T>
  class Metric : GenericMetric {
   public:
    // Sets current metric value to parameter
    void Update(const T& value) { 
      boost::lock_guard<boost::mutex> l(lock_);
      value_ = value;
    }

    // If current value == test_, update with new value. In all cases return
    // current value so that success can be detected. 
    T TestAndSet(const T& value, const T& test) {
      boost::lock_guard<boost::mutex> l(lock_);
      if (value_ == test) {
        value_ = value;
        return test;
      }
      return value_;
    }

    // Reads the current value under the metric lock
    T value() {
      boost::lock_guard<boost::mutex> l(lock_);
      return value_;
    }

    virtual void Print(std::stringstream* out) {
      boost::lock_guard<boost::mutex> l(lock_);
      (*out) << key_ << ":";
      PrintValue(out);
    }    

    virtual void PrintJson(std::stringstream* out) {
      boost::lock_guard<boost::mutex> l(lock_);
      (*out) << "\"" << key_ << "\": ";
      PrintValueJson(out);
    }

    Metric(const std::string& key, const T& value) 
        : value_(value), key_(key) { }

   protected:
    // Subclasses are required to implement this to print a string
    // representation of the metric to the supplied stringstream.
    // Both methods are always called with lock_ taken, so implementations must
    // not try and take lock_ themselves..
    virtual void PrintValue(std::stringstream* out) = 0;
    virtual void PrintValueJson(std::stringstream* out) = 0;

    // Guards access to value
    boost::mutex lock_;
    T value_;

    // Unique key identifying this metric
    const std::string key_;

    friend class Metrics;
  };

  // PrimitiveMetrics are the most common metric type, whose values natively
  // support operator<< and optionally operator+. 
  template<typename T>
  class PrimitiveMetric : public Metric<T> {
   public:
    PrimitiveMetric(const std::string& key, const T& value) 
        : Metric<T>(key, value) {
    }

    // Requires that T supports operator+. Returns value of metric after increment
    T Increment(const T& delta) {
      boost::lock_guard<boost::mutex> l(this->lock_);
      this->value_ += delta;
      return this->value_;
    }

   protected:
    virtual void PrintValue(std::stringstream* out)  {
      (*out) << this->value_;
    }    

    virtual void PrintValueJson(std::stringstream* out)  {
      (*out) << "\"" << this->value_ << "\"";
    }    
  };

  // Convenient typedefs for common primitive metric types.
  typedef struct PrimitiveMetric<int64_t> IntMetric;
  typedef struct PrimitiveMetric<double> DoubleMetric;
  typedef struct PrimitiveMetric<std::string> StringMetric;
  typedef struct PrimitiveMetric<bool> BooleanMetric;

  Metrics();

  // Create a primitive metric object with given key and initial value (owned by
  // this object) If a metric is already registered to this name it will be
  // overwritten (in debug builds it is an error)
  template<typename T>
  PrimitiveMetric<T>* CreateAndRegisterPrimitiveMetric(const std::string& key, 
      const T& value) {
    return RegisterMetric(new PrimitiveMetric<T>(key, value));
  }

  // Registers a new metric. Ownership of the metric will be transferred to this
  // Metrics object, so callers should take care not to destroy the Metric they
  // pass in.
  // If a metric already exists with the supplied metric's key, it is replaced. 
  // The template parameter M must be a subclass of Metric.
  template <typename M>
  M* RegisterMetric(M* metric) {
    boost::lock_guard<boost::mutex> l(lock_);    
    DCHECK(!metric->key_.empty());
    DCHECK(metric_map_.find(metric->key_) == metric_map_.end()) 
      << "Multiple registrations of metric key: " << metric->key_;

    M* mt = obj_pool_->Add(metric);
    metric_map_[metric->key_] = mt;
    return mt;    
  }

  // Register page callbacks with the webserver
  Status Init(Webserver* webserver);

  // Useful for debuggers, returns the output of TextCallback
  std::string DebugString();

 private:
  // Pool containing all metric objects
  boost::scoped_ptr<ObjectPool> obj_pool_;

  // Contains all Metric objects, indexed by key
  typedef std::map<std::string, GenericMetric*> MetricMap;
  MetricMap metric_map_;

  // Guards metric_map_
  boost::mutex lock_;

  // Writes metric_map_ as a list of key : value pairs
  void PrintMetricMap(std::stringstream* output);

  // Builds a list of metrics as Json-style "key": "value" pairs
  void PrintMetricMapAsJson(std::vector<std::string>* metrics);

  // Webserver callback (on /metrics), renders metrics as single text page
  void TextCallback(std::stringstream* output);

  // Webserver callback (on /jsonmetrics), renders metrics as a single json document
  void JsonCallback(std::stringstream* output);
};

}

#endif // IMPALA_UTIL_METRICS_H
