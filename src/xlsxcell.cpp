#include <Rcpp.h>
#include "rapidxml.h"
#include "xlsxbook.h"
#include "xlsxcell.h"
#include "utils.h"

using namespace Rcpp;

xlsxcell::xlsxcell(rapidxml::xml_node<>* c,
    double& height, std::vector<double>& colWidths, xlsxbook& book): 
  c_(c), height_(height), book_(book) {
    r_ = c_->first_attribute("r");
    if (r_ == NULL)
      stop("Invalid cell: lacks 'r' attribute");
    address_ = std::string(r_->value());
    parseAddress(address_, row_, col_);
    width_ = colWidths[col_ - 1]; // row height is provided to the constructor
                                  // whereas col widths have to be looked up

    v_ = c_->first_node("v");
    if (v_ != NULL) {
      vvalue_ = v_->value();
      content_ = vvalue_;
    }

    t_ = c_->first_attribute("t");
    if (t_ != NULL) {
      tvalue_ = t_->value();
      type_ = tvalue_;
    }

    // TODO: Formulas are more complicated than this, because they're shared.
    getChildValueString("f", c_, f_, formula_);

    cacheString(); // t_ and v_ must be obtained first

}

std::string xlsxcell::address() {return(address_);}
int xlsxcell::row() {return(row_);}
int xlsxcell::col() {return(col_);}
String xlsxcell::content() {return content_;}
String xlsxcell::formula() {return formula_;}
String xlsxcell::type() {return type_;}
String xlsxcell::character() {return character_;}
double xlsxcell::height() {return height_;}
double xlsxcell::width() {return width_;}

// Based on readxl
void xlsxcell::cacheString() {
  // If an inline string, it must be parsed, if a string in the string table, it
  // must be obtained.

  // Is it an inline string?  // 18.3.1.53 is (Rich Text Inline) [p1649]
  rapidxml::xml_node<>* is = c_->first_node("is");
  if (is != NULL) {
    std::string inlineString;
    if (!parseString(is, inlineString)) { // value is modified in place
      character_ = NA_STRING;
    } else {
      character_ = inlineString;
    }
    return;
  }

  if (v_ != NULL && t_ != NULL && strncmp(tvalue_, "s", t_->value_size()) == 0) {
    // the t attribute exists and its value is exactly "s", so v_ is an index
    // into the strings table.
    long int v = atol(vvalue_);
    const std::vector<std::string>& strings = book_.strings();
    long int stringssize = book_.strings().size(); // Not actually that expensive
    if (v < 0 || v >= stringssize) {
      warning("[%i, %i]: Invalid string id %i", row_, col_, v);
      character_ = NA_STRING;
      return;
    }
    character_ = strings[v];
    return;
  }
  character_ = NA_STRING;
}