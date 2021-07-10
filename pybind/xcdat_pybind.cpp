#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <xcdat.hpp>

namespace {

class Keyset {
  private:
    std::vector<std::string> m_keys;

  public:
    Keyset() = default;
    virtual ~Keyset() = default;

    void append(const std::string& key) {
        m_keys.push_back(key);
    }

    void complete() {
        std::sort(m_keys.begin(), m_keys.end());
        m_keys.erase(std::unique(m_keys.begin(), m_keys.end()), m_keys.end());
    }

    const std::vector<std::string>& get() const {
        return m_keys;
    }
};

}  // namespace

namespace py = pybind11;

template <class Trie>
void define_Trie(py::module_& m, const std::string& suffix) {
    const std::string name = "Trie" + suffix;
    py::class_<Trie>(m, name.c_str(), "A class of trie dictionary")  //
        .def(py::init([](const Keyset& keyset) {  //
                 return std::make_unique<Trie>(keyset.get());
             }),
             "Build the trie dictionary from the keyset.", py::arg("keyset"))
        .def(py::init([](const std::string& filepath) {  //
                 return std::make_unique<Trie>(xcdat::load<Trie>(filepath));
             }),
             "Load the trie dictionary from the filepath.", py::arg("filepath"))
        .def(
            "save", [](const Trie& self, const std::string& filepath) { xcdat::save(self, filepath); },
            "Save the trie dictionary to the filepath", py::arg("filepath"))
        .def(
            "memory_in_bytes", [](const Trie& self) { return xcdat::memory_in_bytes(self); },
            "Get the memory usage of the trie dictionary")
        .def("bin_mode", &Trie::bin_mode, "Check if the binary mode.")
        .def("num_keys", &Trie::num_keys, "Get the number of stored keywords.")
        .def("alphabet_size", &Trie::alphabet_size, "Get the alphabet size.")
        .def("max_length", &Trie::max_length, "Get the maximum length of keywords.")
        .def("num_nodes", &Trie::num_nodes, "Get the number of trie nodes.")
        .def("num_units", &Trie::num_units, "Get the number of DA units.")
        .def("num_free_units", &Trie::num_free_units, "Get the number of unused DA units.")
        .def("tail_length", &Trie::tail_length, "Get the length of TAIL vector.")
        .def("lookup", &Trie::lookup, "Lookup the ID of the keyword.", py::arg("key"))
        .def(
            "decode", [](const Trie& self, std::uint64_t id) { return self.decode(id); },
            "Decode the keyword associated with the ID.", py::arg("id"))
        .def(
            "make_prefix_iterator",
            [](const Trie& self, const std::string& key) {
                return std::make_unique<typename Trie::prefix_iterator>(self.make_prefix_iterator(key));
            },
            "Make the common prefix searcher for the given keyword.", py::arg("key"))
        .def(
            "make_predictive_iterator",
            [](const Trie& self, const std::string& key) {
                return std::make_unique<typename Trie::predictive_iterator>(self.make_predictive_iterator(key));
            },
            "Make the predictive searcher for the keyword.", py::arg("key"))
        .def(
            "make_enumerative_iterator",
            [](const Trie& self) {
                return std::make_unique<typename Trie::enumerative_iterator>(self.make_enumerative_iterator());
            },
            "Make the enumerator.");
}

template <class Trie>
void define_PrefixIterator(py::module_& m, const std::string& suffix) {
    const std::string name = "PrefixIterator" + suffix;
    py::class_<typename Trie::prefix_iterator>(
        m, name.c_str(),
        "An iterator class for common prefix search.\n"
        "It enumerates all the keywords contained as prefixes of a given string.\n"
        "It should be instantiated via the function 'make_prefix_iterator'.")
        .def("next", &Trie::prefix_iterator::next,
             "Increment the iterator. Return false if the iteration is terminated.")
        .def("id", &Trie::prefix_iterator::id, "Get the result ID.")
        .def("decoded", &Trie::prefix_iterator::decoded, "Get the result keyword.");
}

template <class Trie>
void define_PredictiveIterator(py::module_& m, const std::string& suffix) {
    const std::string name = "PredictiveIterator" + suffix;
    py::class_<typename Trie::predictive_iterator>(
        m, name.c_str(),
        "An iterator class for predictive search or enumeration.\n"
        "It enumerates all the keywords starting with a given string (including an empty string).\n"
        "It should be instantiated via the function 'make_prefix_iterator' or 'make_enumerative_iterator'.")  //
        .def("next", &Trie::predictive_iterator::next,
             "Increment the iterator. Return false if the iteration is terminated.")
        .def("id", &Trie::predictive_iterator::id, "Get the result ID.")
        .def("decoded", &Trie::predictive_iterator::decoded, "Get the result keyword.");
}

PYBIND11_MODULE(xcdat, m) {
    m.doc() = "Xcdat: Fast compressed trie dictionary";

    py::class_<Keyset>(m, "Keyset", "A class to store keywords for building xcdat.Trie.")  //
        .def(py::init<>())
        .def("append", &Keyset::append, "Append the given key.", py::arg("key"))
        .def("complete", &Keyset::complete, "Complete the keyset to build a xcdat.Trie.");

    define_Trie<xcdat::trie_8_type>(m, "8");
    define_PrefixIterator<xcdat::trie_8_type>(m, "8");
    define_PredictiveIterator<xcdat::trie_8_type>(m, "8");

    define_Trie<xcdat::trie_16_type>(m, "16");
    define_PrefixIterator<xcdat::trie_16_type>(m, "16");
    define_PredictiveIterator<xcdat::trie_16_type>(m, "16");

    define_Trie<xcdat::trie_7_type>(m, "7");
    define_PrefixIterator<xcdat::trie_7_type>(m, "7");
    define_PredictiveIterator<xcdat::trie_7_type>(m, "7");

    define_Trie<xcdat::trie_15_type>(m, "15");
    define_PrefixIterator<xcdat::trie_15_type>(m, "15");
    define_PredictiveIterator<xcdat::trie_15_type>(m, "15");
}
