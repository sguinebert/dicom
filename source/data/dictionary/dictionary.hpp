#ifndef DICTIONARY_HPP
#define DICTIONARY_HPP

#include <string>
#include <type_traits>

#include "dictionary_dyn.hpp"
#include "datadictionary.hpp"
#include "commanddictionary.hpp"

namespace dicom
{

namespace data
{

namespace dictionary
{
static dictionary_entry unknown {{attribute::VR::UN}, "UNKNOWN", "UNKNOWN", "*", false};

/**
 * @brief The dictionary class is a facade for the different kind of
 *        dictionaries.
 */
class dictionaries
{
   public:
      template <unsigned short gid>
      using static_gid = std::integral_constant<unsigned short, gid>;
      template <unsigned short eid>
      using static_eid = std::integral_constant<unsigned short, eid>;


      /**
       * @brief The dictionary constructor initializes the dynamic dictionaries
       *        to the paths specified in the parameters.
       * @param cmddic_path path to the command dictionary
       * @param datadic_path path toe the data dictionary
       */
      dictionaries(std::string cmddic_path = "commanddictionary.csv",
                   std::string datadic_path = "datadictionary.csv"):
          commanddic {cmddic_path, dictionary_dyn::MODE::GREEDY},
          datadic {datadic_path}
      {
      }


      /**
       * @brief lookup_commanddic performs a dynamic lookup of the given tag in
       *        the command dictionary.
       * @param tag
       * @return dictionary entry corresponding the tag, if not found, unknown
       *         placeholder
       * We do not return a boost::optional here because the clients may not
       * want to do an extra check for an unlikely result. The "unknown" tag
       * in case of an error has no constraints and the associated data element
       * can still be processed.
       */
      dictionary_entry lookup_commanddic(attribute::tag_type tag)
      {
          auto entry = commanddic.lookup(tag);
          if (entry == boost::none) {
              return unknown;
          } else {
              return *entry;
          }
      }

      /**
       * @brief lookup_datadic performs a dynamic lookup of the given tag in
       *        the data dictionary.
       * @param tag
       * @return dictionary entry corresponding the tag
       */
      dictionary_entry lookup_datadic(attribute::tag_type tag)
      {
          auto entry = datadic.lookup(tag);
          if (entry == boost::none) {
              return unknown;
          } else {
              return *entry;
          }
      }

      /**
       * @brief lookup performs a lookup in the command dictionary and data
       *        dictionar respectively
       * @param gid group id of the tag
       * @param eid element id of the tag
       * @return dictionary entry corresponding the tag
       */
      dictionary_entry lookup(attribute::tag_type tag)
      {
          try {
              auto found_entry {commanddic.lookup(tag)};
              if (found_entry == boost::none) {
                  auto found_data_entry {datadic.lookup(tag)};
                  if (found_data_entry == boost::none) {
                      return unknown;
                  } else {
                      return *found_data_entry;
                  }
              }

              return *found_entry;
          } catch (std::exception&) {
              return unknown;
          }
      }

      /**
       * @brief lookup_commanddic performs a compile-time lookup of the given tag
       *        in the command dictionary.
       * @param gid group id of the tag
       * @param eid element id of the tag
       * @return dictionary entry corresponding the tag
       */
      template <unsigned short g, unsigned short e>
      dictionary_entry lookup_commanddic(std::integral_constant<unsigned short, g>,
                                         std::integral_constant<unsigned short, e>)
      {
         return commanddictionary<g, e> {};
      }

      /**
       * @brief lookup_datadic performs a compile-time lookup of the given tag
       *        in the data dictionary.
       * @param gid group id of the tag
       * @param eid element id of the tag
       * @return dictionary entry corresponding the tag
       */
      template <unsigned short g, unsigned short e>
      dictionary_entry lookup_datadic(std::integral_constant<unsigned short, g>,
                                      std::integral_constant<unsigned short, e>)
      {
         return datadictionary<g, e> {};
      }

      dictionary_dyn& get_dyn_commanddic()
      {
          return commanddic;
      }
      dictionary_dyn& get_dyn_datadic()
      {
          return datadic;
      }

   private:
      dictionary_dyn commanddic;
      dictionary_dyn datadic;
};

/**
 * @brief get_default_dictionaries returns a reference to a standard
 *        dictionaries instance containing the command and data dictionaries.
 * @return dictionary instance
 */
dictionaries& get_default_dictionaries()
{
    static dictionaries dict {"commanddictionary.csv", "datadictionary.csv"};
    return dict;
}


}

}

}

#endif // DICTIONARY_HPP
