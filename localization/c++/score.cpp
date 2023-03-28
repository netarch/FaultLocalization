#include "score.h"
#include "utils.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>

using namespace std;

void Score::SetLogData(LogData *data_, double max_finish_time_ms,
                       int nopenmp_threads) {
    Estimator::SetLogData(data_, max_finish_time_ms, nopenmp_threads);
    if (!USE_PASSIVE) {
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
}

Score *Score::CreateObject() {
    Score *ret = new Score();
    vector<double> param{fail_threshold};
    ret->SetParams(param);
    return ret;
}

void Score::SetParams(vector<double> &params) {
    assert(params.size() == 1);
    fail_threshold = params[0];
}

PDD Score::ComputeScoresPassive(vector<double> &scores,
                                Hypothesis &problematic_link_ids,
                                double min_start_time_ms,
                                double max_finish_time_ms) {
    int nlinks = scores.size();
    vector<double> scores1(nlinks, 0);
    vector<double> scores2(nlinks, 0);
    for (int ff = 0; ff < data->flows.size(); ff++) {
        Flow *flow = data->flows[ff];
        double score1 =
            ((double)flow->LabelWeightsFunc(max_finish_time_ms).second);
        double score2 =
            ((double)flow->LabelWeightsFunc(max_finish_time_ms).first);
        if (flow->TracerouteFlow(max_finish_time_ms)) {
            Path *path_taken = flow->GetPathTaken();
            if (!HypothesisIntersectsPath(&problematic_link_ids, path_taken) and
                (problematic_link_ids.count(flow->first_link_id) == 0) and
                (problematic_link_ids.count(flow->last_link_id) == 0)) {
                int total_path_length = path_taken->size() + 2;
                scores1[flow->first_link_id] += score1;
                scores2[flow->first_link_id] += score2;
                scores1[flow->last_link_id] += score1;
                scores2[flow->last_link_id] += score2;
                for (int link_id : *path_taken) {
                    scores1[link_id] += score1;
                    scores2[link_id] += score2;
                }
            }
        } else {
            assert(score1 < 1.0e-6);
            vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
            score2 /= flow_paths->size();
            for (Path *path : *flow_paths) {
                if (!HypothesisIntersectsPath(&problematic_link_ids, path) and
                    (problematic_link_ids.count(flow->first_link_id) == 0) and
                    (problematic_link_ids.count(flow->last_link_id) == 0)) {
                    scores2[flow->first_link_id] += score2;
                    scores2[flow->last_link_id] += score2;
                    for (int link_id : *path) {
                        scores2[link_id] += score2;
                    }
                }
            }
        }
    }
    for (int i = 0; i < nlinks; i++)
        scores[i] = scores1[i] / max(1.0e-9, scores2[i]);
    double sum_scores =
        accumulate(scores.begin(), scores.end(), 0.0, plus<double>());
    double max_scores = *max_element(scores.begin(), scores.end());
    return PDD(sum_scores, max_scores);
}

PDD Score::ComputeScores(vector<Flow *> &bad_flows, vector<double> &scores,
                         Hypothesis &problematic_link_ids,
                         double min_start_time_ms, double max_finish_time_ms) {
    fill(scores.begin(), scores.end(), 0.0);
    /*
    int nlinks = scores.size();
    vector<double> scores1(nlinks, 0);
    vector<double> scores2(nlinks, 0);
    */
    for (int ff = 0; ff < bad_flows.size(); ff++) {
        Flow *flow = bad_flows[ff];
        assert(flow->TracerouteFlow(max_finish_time_ms));
        Path *path_taken = flow->GetPathTaken();
        if (!HypothesisIntersectsPath(&problematic_link_ids, path_taken) and
            (problematic_link_ids.count(flow->first_link_id) == 0) and
            (problematic_link_ids.count(flow->last_link_id) == 0)) {
            int flow_score =
                (int)(flow->LabelWeightsFunc(max_finish_time_ms).second > 0);
            assert(flow_score == 1);
            int total_path_length = path_taken->size() + 2;
            double score = ((double)flow_score) / total_path_length;

            /*
            double score1 = ((double)
            flow->LabelWeightsFunc(max_finish_time_ms).second); double score2 =
            ((double) flow->LabelWeightsFunc(max_finish_time_ms).first);
            scores1[flow->first_link_id] += score1; scores2[flow->first_link_id]
            += score2; scores1[flow->last_link_id] += score1;
            scores2[flow->last_link_id] += score2;
            */

            scores[flow->first_link_id] += score;
            scores[flow->last_link_id] += score;
            for (int link_id : *path_taken) {
                scores[link_id] += score;
                // scores1[link_id] += score1; scores2[link_id] += score2;
            }
        }
    }
    // for (int i=0; i<nlinks; i++) scores[i] = scores1[i]/max(1.0e-9,
    // scores2[i]);
    double sum_scores =
        accumulate(scores.begin(), scores.end(), 0.0, plus<double>());
    double max_scores = *max_element(scores.begin(), scores.end());
    return PDD(sum_scores, max_scores);
}

void Score::LocalizeFailures(double min_start_time_ms,
                             double max_finish_time_ms,
                             Hypothesis &localized_links, int nopenmp_threads) {
    assert(data != NULL);
    localized_links.clear();
    vector<Flow *> bad_flows;
    for (Flow *flow : data->flows) {
        if (flow->TracerouteFlow(max_finish_time_ms) > 0)
            bad_flows.push_back(flow);
    }
    int nlinks = data->inverse_links.size();
    vector<double> scores(nlinks, 0.0);
    if (VERBOSE) {
        cout << "Num flows " << bad_flows.size();
    }
    double sum_scores, max_scores;
    if (USE_PASSIVE) {
        tie(sum_scores, max_scores) = ComputeScoresPassive(
            scores, localized_links, min_start_time_ms, max_finish_time_ms);
    } else {
        tie(sum_scores, max_scores) =
            ComputeScores(bad_flows, scores, localized_links, min_start_time_ms,
                          max_finish_time_ms);
    }
    if (VERBOSE) {
        cout << " sumscore " << sum_scores << " maxscore " << max_scores
             << endl;
    }
    while ((USE_ABSOLUTE_THRESHOLD and (max_scores >= fail_threshold)) or
           (!USE_ABSOLUTE_THRESHOLD and
            (max_scores >= fail_threshold * sum_scores and max_scores > 0))) {
        if (VERBOSE) {
            cout << "Num flows " << bad_flows.size();
        }
        int faulty_link_id =
            distance(scores.begin(), max_element(scores.begin(), scores.end()));
        localized_links.insert(faulty_link_id);
        if (ADJUST_VOTES) {
            if (USE_PASSIVE) {
                tie(sum_scores, max_scores) =
                    ComputeScoresPassive(scores, localized_links,
                                         min_start_time_ms, max_finish_time_ms);
            } else {
                tie(sum_scores, max_scores) =
                    ComputeScores(bad_flows, scores, localized_links,
                                  min_start_time_ms, max_finish_time_ms);
            }
        } else {
            scores[faulty_link_id] =
                -1000000; // Ensure it doesn't show up again
            sum_scores =
                accumulate(scores.begin(), scores.end(), 0.0, plus<double>());
            max_scores = *max_element(scores.begin(), scores.end());
        }
        if (VERBOSE) {
            cout << " sumscore " << sum_scores << " maxscore " << max_scores
                 << endl;
        }
    }
}
