// Copyright (c) 2018 ISciences, LLC.
// All rights reserved.
//
// This software is licensed under the Apache License, Version 2.0 (the "License").
// You may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef EXACTEXTRACT_RASTER_STATS_H
#define EXACTEXTRACT_RASTER_STATS_H

#include <algorithm>
#include <limits>
#include <unordered_map>

#include "raster_cell_intersection.h"

namespace exactextract {

    template<typename T>
    class RasterStats {

    public:
        /**
         * Compute raster statistics from the results of a RasterCellIntersection
         * and a set of raster values.
         *
         * A NODATA value may optionally be provided in addition to NaN.
         */
        RasterStats(const Raster<float> & intersection_percentages, const Raster<T> & rast, const T* nodata = nullptr) :
                m_min{std::numeric_limits<T>::max()},
                m_max{std::numeric_limits<T>::lowest()},
                m_weights{0},
                m_weighted_vals{0},
                m_nodata{nodata} {

            RasterView<T> rv = RasterView<T>{rast, intersection_percentages.extent()};

            for (size_t i = 0; i < rv.rows(); i++) {
                for (size_t j = 0; j < rv.cols(); j++) {
                    float w = intersection_percentages(i, j);
                    T val = rv(i, j);

                    if (w > 0 && !(std::is_floating_point<T>::value && std::isnan(val)) &&
                        (m_nodata == nullptr || val != *m_nodata)) {
                        process(val, w);
                    }
                }
            }
        }

        RasterStats(const Raster<float> & intersection_percentages, const Raster<T> & rast, const Raster<T> weights, const T* nodata = nullptr) :
                m_min{std::numeric_limits<T>::max()},
                m_max{std::numeric_limits<T>::lowest()},
                m_weights{0},
                m_weighted_vals{0},
                m_nodata{nodata} {

            RasterView<T> rv = RasterView<T>{rast, intersection_percentages.extent()};
            RasterView<T> wv = RasterView<T>{rast, intersection_percentages.extent()};

            for (size_t i = 0; i < rv.rows(); i++) {
                for (size_t j = 0; j < rv.cols(); j++) {
                    float w = intersection_percentages(i, j)*wv(i, j);
                    T val = rv(i, j);

                    if (w > 0 && !(std::is_floating_point<T>::value && std::isnan(val)) &&
                        (m_nodata == nullptr || val != *m_nodata)) {
                        process(val, w);
                    }
                }
            }
        }

        /**
         * The mean value of cells covered by this polygon, weighted
         * by the percent of the cell that is covered.
         */
        float mean() {
            return sum() / count();
        }

        /**
         * The raster value occupying the greatest number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the greatest value will
         * be returned.
         */
        T mode() {
            return std::max_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The minimum value in any raster cell wholly or partially covered
         * by the polygon.
         */
        T min() {
            return m_min;
        }

        /**
         * The maximum value in any raster cell wholly or partially covered
         * by the polygon.
         */
        T max() {
            return m_max;
        }

        /**
         * The weighted sum of raster cells covered by the polygon.
         */
        float sum() {
            return (float) m_weighted_vals;
        }

        /**
         * The number of raster cells with a defined value
         * covered by the polygon.
         */
        float count() {
            return (float) m_weights;
        }

        /**
         * The raster value occupying the least number of cells
         * or partial cells within the polygon. When multiple values
         * cover the same number of cells, the lowest value will
         * be returned.
         */
        T minority() {
            return std::min_element(m_freq.cbegin(),
                                    m_freq.cend(),
                                    [](const auto &a, const auto &b) {
                                        return a.second < b.second || (a.second == b.second && a.first < b.first);
                                    })->first;
        }

        /**
         * The number of distinct defined raster values in cells wholly
         * or partially covered by the polygon.
         */
        size_t variety() {
            return m_freq.size();
        }

    private:
        T m_min;
        T m_max;

        double m_weights;
        double m_weighted_vals;

        std::unordered_map<T, float> m_freq;

        const T* m_nodata;

        void process(const T& val, float weight) {
            m_weights += weight;
            m_weighted_vals += weight * val;

            if (val < m_min) {
                m_min = val;
            }

            if (val > m_max) {
                m_max = val;
            }

            m_freq[val] += weight;
        }
    };

}

#endif