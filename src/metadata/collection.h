/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Gregory Sapunkov.

  Portions created by the original developer
  are Copyright (C) 2004 Gregory Sapunkov.

  All Rights Reserved.

  Contributor(s): Milan Babuskov.
*/
//
//
//
//
//------------------------------------------------------------------------------
#ifndef FR_COLLECTION_H
#define FR_COLLECTION_H

#include <list>
#include <sstream>
#include "domain.h"
#include "metadataitem.h"
//------------------------------------------------------------------------------
template <class T>
class YMetadataCollection: public YxMetadataItem
{
public:
	typedef typename std::list<T> ContainerType;
	typedef typename ContainerType::iterator iterator;
	typedef typename ContainerType::const_iterator const_iterator;

 	void remove(T *item)		// removes item from vector
	{
		if (!item)
			return;

		for (iterator pos = itemsM.begin(); pos != itemsM.end(); ++pos)
		{
			if (&(*pos) == item)
			{
				itemsM.erase(pos);
				notify();
				return;
			}
		}
	}

	virtual T* add(T& item)			// adds item to vector, returns pointer to it
	{
		item.setParent(this);
		itemsM.push_back(item);
		notify();
		return &itemsM.back();
	}

	//! same as add() but watches for sorting
	virtual T* add(std::string name)			// inserts item to vector and returns pointer to it
	{
		T item;
		item.setParent(this);

		iterator pos = itemsM.begin();		// find the place
		for (; pos != itemsM.end(); ++pos)
		{
			YxMetadataItem *p = &(*pos);
			if (!p)
				continue;
			if (p->getName() > name)
				break;
		}

		pos = itemsM.insert(pos, item);
		return &(*pos);
	}

	virtual T* add()				// Creates new item, adds it and returns pointer to it.
	{								// notify() is *not* called since newly added object still doesn't
		T item;						// have its properties set, so for example getName() would return
		item.setParent(this);		// empty string. It is responsibility of the caller to call notify
		itemsM.push_back(item);		// after it has set the properties.
		return &itemsM.back();
	}

	inline const_iterator begin() const
	{
		return itemsM.begin();
	};

	inline const_iterator end() const
	{
		return itemsM.end();
	};

	inline bool empty() const
	{
		return itemsM.empty();
	};

	inline void clear()
	{
		itemsM.clear();
		notify();
	};

	virtual YxMetadataItem *findByName(std::string name)
	{
		for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
		{
			YxMetadataItem *p = &(*it);
			if (!p)
				continue;
			if (p->getName() == name)
				return p;
		}
		return 0;
	};

	virtual bool getChildren(std::vector<YxMetadataItem *>& temp)			// returns vector of all subnodes
	{
		for (iterator it = itemsM.begin(); it != itemsM.end(); ++it)
			temp.push_back(&(*it));
		return (itemsM.size() != 0);
	}

	void getChildrenNames(std::vector<std::string>& temp)
	{
		for (const_iterator it = itemsM.begin(); it != itemsM.end(); ++it)
			temp.push_back((*it).getName());
	}

	std::string getCreateSqlTemplate() const		// this could be done better, but I haven't got the idea
	{												// function looks like it could be static, but then it
		T dummy;									// can't be virtual, and vice versa. So I just create a dummy
		return dummy.getCreateSqlTemplate();		// object to call the function on.
	}

	virtual std::string getPrintableName() const
	{
		if (typeM != ntDomains)
			return YxMetadataItem::getPrintableName();

		unsigned int n = 0;
		for (const_iterator it = itemsM.begin(); it != itemsM.end(); ++it)
			if (!(*it).isSystem())
				n++;
		if (n)
		{
			std::ostringstream ss;
			ss << nameM << " (" << n << ")";
			return ss.str();
		}
		else
			return nameM;
	}

	virtual size_t getChildrenCount() const
	{
		return itemsM.size();
 	}

private:
	ContainerType itemsM;

};
//------------------------------------------------------------------------------
/* FIXME: from some yet unknown reason, this doesn't compile on g++ 3.3
//! specific for domains since system-generated domains should not be counted
template<>
std::string YMetadataCollection<YDomain>::getPrintableName() const
{
	unsigned int n = 0;
	for (const_iterator it = itemsM.begin(); it != itemsM.end(); ++it)
		if (!(*it).isSystem())
			n++;
 	if (n)
	{
		std::ostringstream ss;
		ss << nameM << " (" << n << ")";
		return ss.str();
	}
	else
		return nameM;
} */
//------------------------------------------------------------------------------
#endif
