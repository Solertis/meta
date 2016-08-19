/**
 * @file feature_selection_test.cpp
 * @author Sean Massung
 */

#include "bandit/bandit.h"
#include "create_config.h"
#include "meta/classify/multiclass_dataset.h"
#include "meta/classify/multiclass_dataset_view.h"
#include "meta/features/all.h"
#include "meta/features/feature_selector.h"
#include "meta/features/selector_factory.h"

using namespace bandit;
using namespace meta;

namespace {

template <class DatasetView>
void run_test(DatasetView& dset_vw, const std::string& method_id, term_id tid) {
    auto config = cpptoml::make_table();
    auto fcfg = cpptoml::make_table();
    fcfg->insert("method", method_id);
    fcfg->insert("prefix", "test-features");
    config->insert("features", fcfg);

    auto selector = features::make_selector(*config, dset_vw);
    selector->select(20);
    selector->select(50);
    selector->select_percent(0.05);
    selector->select_percent(0.10);

    AssertThat(selector->selected(tid), IsTrue());

    for (uint64_t lbl_id = 0; lbl_id < dset_vw.total_labels(); ++lbl_id)
        AssertThat(filesystem::file_exists("test-features." + method_id + "."
                                           + std::to_string(lbl_id + 1)),
                   IsTrue());

    AssertThat(
        filesystem::file_exists("test-features." + method_id + ".selected"),
        IsTrue());
}

template <class Index>
void test_method(Index& idx, const std::string& method_id) {

    classify::multiclass_dataset dset{idx};
    classify::multiclass_dataset_view dset_vw(dset);

    auto tid = idx->get_term_id("china"); // this term should be selected

    run_test(dset_vw, method_id, tid);
}

template <class Index>
void test_method_binary(Index& idx, const std::string& method_id) {
    classify::binary_dataset dset{
        idx, [&](doc_id did) { return idx->label(did) == "chinese"_cl; }};
    classify::binary_dataset_view dset_vw{dset};

    auto tid = idx->get_term_id("china"); // this term should be selected

    run_test(dset_vw, method_id, tid);
}
}

go_bandit([]() {
    auto line_cfg = tests::create_config("line");
    auto f_idx = index::make_index<index::memory_forward_index>(*line_cfg);

    // run each test twice to ensure files can be read from disk
    describe("[feature-selection]", [&]() {

        it("should implement chi square", [&]() {
            test_method(f_idx, "chi-square");
            test_method(f_idx, "chi-square");
        });

        it("should implement information gain", [&]() {
            test_method(f_idx, "info-gain");
            test_method(f_idx, "info-gain");
        });

        it("should implement correlation coefficient", [&]() {
            test_method(f_idx, "corr-coef");
            test_method(f_idx, "corr-coef");
        });

        it("should implement odds ratio", [&]() {
            test_method(f_idx, "odds-ratio");
            test_method(f_idx, "odds-ratio");
        });

        for (const std::string& method_id :
             {"chi-square", "info-gain", "corr-coef", "odds-ratio"}) {
            for (const std::string& suffix : {"1", "2", "3", "selected"}) {
                filesystem::remove_all("test-features." + method_id + "."
                                       + suffix);
            }
        }

        it("should implement chi square (binary)", [&]() {
            test_method_binary(f_idx, "chi-square");
            test_method_binary(f_idx, "chi-square");
        });

        it("should implement information gain (binary)", [&]() {
            test_method_binary(f_idx, "info-gain");
            test_method_binary(f_idx, "info-gain");
        });

        it("should implement correlation coefficient (binary)", [&]() {
            test_method_binary(f_idx, "corr-coef");
            test_method_binary(f_idx, "corr-coef");
        });

        it("should implement odds ratio (binary)", [&]() {
            test_method_binary(f_idx, "odds-ratio");
            test_method_binary(f_idx, "odds-ratio");
        });

        for (const std::string& method_id :
             {"chi-square", "info-gain", "corr-coef", "odds-ratio"}) {
            for (const std::string& suffix : {"1", "2", "selected"}) {
                filesystem::remove_all("test-features." + method_id + "."
                                       + suffix);
            }
        }

    });

    f_idx = nullptr;
    filesystem::remove_all("ceeaus");
});
