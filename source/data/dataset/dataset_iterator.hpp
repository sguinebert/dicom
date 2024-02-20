#ifndef DATASET_ITERATOR_HPP
#define DATASET_ITERATOR_HPP

#include <iterator>
#include <map>
#include <stack>

#include "datasets.hpp"
#include "data/attribute/attribute.hpp"
#include "data/dictionary/dictionary.hpp"

#include "data/attribute/constants.hpp"

namespace dicom
{

namespace data
{

namespace dataset
{
using namespace attribute;
using namespace data::dictionary;

/**
 * @brief The dataset iterator provides an easy-to-use interface for
 *        traversing a DICOM dataset bidirectionally.
 * The dataset, which spans a tree with its nested sequences, is traversed in a
 * depth-first-order. When a sequence item is encountered, its subset is pushed
 * onto a stack. The increment and decrement operations operate on the set on
 * the top of the stack, and will pop it off if the sequence is finished to
 * continue to traverse the sequence-containing set.
 */
class dataset_iterator: public std::iterator<std::bidirectional_iterator_tag, std::pair<attribute::tag_type, attribute::elementfield>>
{
   public:
    dataset_iterator(std::map<attribute::tag_type, attribute::elementfield>::iterator it):
        cit {it},
        delimiter {it},
        dictionary {get_default_dictionaries()}
    {
        nested_set_sizes.push({0, 0});
    }

    dataset_iterator operator++()
    {
        return next();
    }

    dataset_iterator operator++(int)
    {
        dataset_iterator ret = cit;
        cit = next();
        return ret;
    }

    dataset_iterator operator--()
    {
        return previous();
    }

    dataset_iterator operator--(int)
    {
        dataset_iterator ret = cit;
        cit = previous();
        return ret;
    }


    std::pair<attribute::tag_type, attribute::elementfield>
    operator*() const
    {
        return *cit;
    }


    std::pair<const attribute::tag_type, attribute::elementfield> const*
    operator->() const
    {
        return &*cit;
    }



      // dataset_iterator operator++();
      // dataset_iterator operator++(int);

      // dataset_iterator operator--();
      // dataset_iterator operator--(int);

      // std::pair<attribute::tag_type, attribute::elementfield>
      // operator*() const;

      // std::pair<const attribute::tag_type, attribute::elementfield> const*
      // operator->() const;

      friend bool operator==(const dataset_iterator& lhs, const dataset_iterator& rhs);

   private:
      std::map<attribute::tag_type, attribute::elementfield>::iterator cit;
      std::map<attribute::tag_type, attribute::elementfield>::iterator delimiter;

      std::stack<typename attribute::type_of<attribute::VR::SQ>::type> nested_sets;
      std::stack<std::map<attribute::tag_type, attribute::elementfield>::iterator> parent_its;

      /**
       * @brief The set_size struct is a pod to hold data of the accumulated
       *        size and the maximum size of a sequence.
       */
      struct set_size
      {
            std::size_t curr_nestedset_size;
            std::size_t curr_nestedset_max;
      };

      std::stack<set_size> nested_set_sizes;

      struct nested_items
      {
            std::size_t nested_items_curr;
            std::size_t nested_items_max;
      };

      std::stack<nested_items> items;

      dictionary::dictionaries& dictionary;

      /**
       * @brief step_into_nested is called by next() if a sequence item is
       *        encountered. The subset is pushed onto the top of the set stack.
       * @param curr set-iterator pointing to the sequence item.
       * @return iterator to the first element of the nested set.
       */
      std::map<attribute::tag_type, attribute::elementfield>::iterator
      step_into_nested(std::map<attribute::tag_type, attribute::elementfield>::iterator curr)
      {
          parent_its.push(curr); // save current position of the iterator
          nested_set_sizes.push({0, curr->second.value_len});
          std::vector<dataset_type> nested_set;
          get_value_field<VR::SQ>(curr->second, nested_set);

          nested_set.erase(std::remove_if(nested_set.begin(), nested_set.end(),
                                          [](const dataset_type& set) { return set.empty(); }),
                           nested_set.end());

          nested_sets.push(nested_set);
          items.push({0, nested_sets.top().size()});
          if (nested_set.size() == 0 || /* empty set*/ nested_set[0].size() == 0) {
              return step_outof_nested();
          } else {
              curr = nested_sets.top().begin()->begin();
              return curr;
          }
      }

      /**
       * @brief step_outof_nested is called when the nested set is completely
       *        traversed. The subset is popped off the stack.
       * @return iterator to the first element after the set.
       * A nested sequence is finished if either a delimitation item is
       * encountered (tag = (0xfffe, 0xe0dd)) or the accumulated size of
       * traversed items is larger than the size specified in the respective SQ
       * item.
       */
      std::map<attribute::tag_type, attribute::elementfield>::iterator step_outof_nested()
      {
          auto last = parent_its.top();
          parent_its.pop();
          nested_sets.pop();
          items.pop();
          nested_set_sizes.pop();
          return ++last;
      }

      /**
       * @brief step_backw_into_nested is called if the previous item is a
       *        sequence item, which then will be pushed onto the set stack.
       * @param prev set-iterator pointing to the sequence item.
       * @return iterator to the last element of the nested set.
       */
      std::map<attribute::tag_type, attribute::elementfield>::iterator
      step_backw_into_nested(std::map<attribute::tag_type, attribute::elementfield>::iterator prev)
      {
          bool explicitlength = true;
          parent_its.push(prev);
          nested_set_sizes.push({prev->second.value_len, prev->second.value_len});
          if (nested_set_sizes.top().curr_nestedset_max != 0xffff) {
              nested_set_sizes.top().curr_nestedset_size = nested_set_sizes.top().curr_nestedset_max;
          } else {
              explicitlength = false;
          }
          std::vector<dataset_type> nested_set;
          get_value_field<VR::SQ>(prev->second, nested_set);
          nested_sets.push(nested_set);
          items.push({nested_sets.size()-1, nested_sets.size()});
          auto curr = --nested_sets.top()[items.top().nested_items_max-1].end();
          return explicitlength ? curr : --curr;
      }

      /**
       * @brief step_backw_outof_nested is called when the nested set is
       *        completely traversed backwards. The subset is popped off the
       *        stack.
       * @return iterator to the element before the sequence.
       */
      std::map<attribute::tag_type, attribute::elementfield>::iterator step_backw_outof_nested()
      {
          auto last = parent_its.top();
          parent_its.pop();
          nested_sets.pop();
          items.pop();
          nested_set_sizes.pop();
          return last;
      }

      std::map<attribute::tag_type, attribute::elementfield>::iterator next()
      {
          // accumulate the size of the nested set's elements if a sequence size was
          // explicitly specified
          if (nested_set_sizes.top().curr_nestedset_max != 0xffff
              && cit->first != Item
              && cit->first != ItemDelimitationItem
              && cit->first != SequenceDelimitationItem) {
              nested_set_sizes.top().curr_nestedset_size += cit->second.value_len + 4 + 4;
          }

          /// @todo check what happens when the last item in a sequence is another SQ
          /// -> maybe put else if branch outside as a "standalone" if.
          if (cit->first == SequenceDelimitationItem || cit->first == ItemDelimitationItem
              || (is_in_nested() && nested_set_sizes.top().curr_nestedset_max != 0xffffffff  && (cit->second.value_rep != VR::SQ || dictionary.lookup(cit->first).vr[0] != VR::SQ) && nested_set_sizes.top().curr_nestedset_size >= nested_set_sizes.top().curr_nestedset_max)) {
              //      || (is_in_nested() && cit->second.value_rep != VR::SQ && nested_set_sizes.top().curr_nestedset_size >= nested_set_sizes.top().curr_nestedset_max)) {
              // sequence delimitation item encountered, check if there are more items.
              // Otherwise, step out of the current sequence.
              auto& nes_items = items.top().nested_items_curr;
              //last = cit->first;
              if (nes_items < items.top().nested_items_max-1) {
                  nes_items++;
                  return (cit = nested_sets.top()[nes_items].begin());
              } else if (cit->first == ItemDelimitationItem) {
                  return ++cit; // we still have the sequenceDelimitationItem
              } else {
                  delimiter = cit = step_outof_nested();
                  return delimiter;
              }
          } else if (cit->second.value_rep.is_initialized()) {
              //last = cit->first;
              // found another sequence in the current set; step into it.
              if (cit->second.value_rep == VR::SQ) {
                  cit = step_into_nested(cit);
                  return cit;
              }
          } else if (dictionary.lookup(cit->first).vr[0] == VR::SQ) {
              // found another sequence in the current set; step into it.
              cit = step_into_nested(cit);
              return cit;
          }

          return ++cit;
      }
      std::map<attribute::tag_type, attribute::elementfield>::iterator previous()
      {
          auto curr = cit;
          --cit;
          if (is_in_nested() &&
              std::any_of(nested_sets.top().begin(), nested_sets.top().end(), [curr](const dataset_type& set) { return set.begin() == curr; })) {
              // current iterator points into a sequence, at the beginning of a item
              // vector. Check if there are more previous elements in the item vector,
              // if not step backwards out of the sequence.
              if (items.top().nested_items_curr > 0) {
                  items.top().nested_items_curr--;
                  return (cit = (nested_sets.top()[items.top().nested_items_curr].end()--));
              } else {
                  return (cit = step_backw_outof_nested());
              }
          } else if ((cit->second.value_rep.is_initialized() && cit->second.value_rep == VR::SQ) ||
                     dictionary.lookup(cit->first).vr[0] == VR::SQ) {
              // previous item will be a sequence, step backwards into it.
              return (cit = step_backw_into_nested(cit));
          }

          if (nested_set_sizes.top().curr_nestedset_max != 0xffff
              && cit->first != Item
              && cit->first != ItemDelimitationItem
              && cit->first != SequenceDelimitationItem) {
              nested_set_sizes.top().curr_nestedset_size -= cit->second.value_len - (4 + 4);
          }

          return cit;
      }

      /**
       * @brief is_in_nested checks if the iterator points into a sequence item.
       * @return true if iterator points into a sequence item, false otherwise.
       */
      bool is_in_nested() const
      {
          return parent_its.size() > 0;
      }
};

bool operator==(const dataset_iterator& lhs, const dataset_iterator& rhs)
{
    return lhs.cit == rhs.cit;
}

bool operator!=(const dataset_iterator& lhs, const dataset_iterator& rhs)
{
    return !(lhs == rhs);
}
/**
 * @brief The dataset_iterator_adaptor class is used as a wrapper for IODs or
 *        command data such that range-based for loops will select the
 *        dataset_iterator instead of the normal set iterator.
 */
class dataset_iterator_adaptor
{
   public:
      dataset_iterator_adaptor(std::map<attribute::tag_type, attribute::elementfield> ds)
           : dataset {ds}
       {
       }

       dataset_iterator begin()
       {
           return dataset.begin();
       }
       dataset_iterator end()
       {
           return dataset.end();
       }

   private:
      std::map<attribute::tag_type, attribute::elementfield> dataset;
};

/**
 * @brief operator << prints the dataset in a human - readable form
 * @param os output stream
 * @param data dataset to print
 * @return modified stream the dataset was printed to
 */
std::ostream& operator<<(std::ostream& os, const dataset_type& data)
{
    int depth = 0;
    traverse(data, [&](attribute::tag_type tag, const attribute::elementfield& ef) mutable
             {
                 if (tag == ItemDelimitationItem) {
                     --depth;
                 }

                 std::fill_n(std::ostream_iterator<char>(os), depth, '\t');
                 os << tag << " ";
                 if (tag != SequenceDelimitationItem
                     && tag != ItemDelimitationItem
                     && tag != Item
                     && ef.value_rep.is_initialized()
                     && ef.value_rep.get() != VR::NN
                     && ef.value_rep.get() != VR::NI) {
                     if (ef.value_rep.get() == VR::SQ ) {
                         os << " " << ef.value_len;
                         if ((ef.value_len & 0xffffffff) == 0xffffffff) {
                             os << "(undefined length)";
                         }
                         os << "\t\t";
                     } else {
                         os << " " << ef.value_len << "\t\t";
                         ef.value_field->print(os);
                     }
                 }
                 os << "\n";

                 if (tag == Item) {
                     ++depth;
                 }
             });
    return os;
}

}

}

}

#endif // DATASET_ITERATOR_HPP
