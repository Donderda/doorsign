/*
 * Helpers.h
 *
 *  Created on: 26 Jun 2018
 *      Author: friedemannstoffregen
 */

#ifndef MAIN_UTILS_HELPERS_H_
#define MAIN_UTILS_HELPERS_H_
#include <string>

class Helpers {
public:
	static void find_and_replace(std::string& source, std::string const& find,
			std::string const& replace) {
		for (std::string::size_type i = 0;
				(i = source.find(find, i)) != std::string::npos;) {
			source.replace(i, find.length(), replace);
			i += replace.length();
		}
	}

	Helpers();
	virtual ~Helpers();
};

#endif /* MAIN_UTILS_HELPERS_H_ */
