#ifndef FACTORY_H__
#define FACTORY_H__

#include <map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>

template<typename Func> class FactoryOwner
{
	private:
		struct less
		{
			bool operator()(const std::string &l, const std::string &r) const
			{
				return boost::algorithm::ilexicographical_compare(l,r);
			}
		};
		typedef std::map<std::string, Func, less> Data;
		Data _data;
	public:
		bool Register(const std::string &name, Func f)
		{
			if (_data.find(name) != _data.end())
					return false;
			_data[name] = f;
			return true;
		}
		Func operator[](const std::string &name) const
		{
				auto i = _data.find(name);
				return i->second;
		}
		bool Exists(const std::string &name) const
		{
			return (_data.find(name) != _data.end());
		}

		typedef typename Data::const_iterator const_iterator;
		const_iterator begin() const{return _data.begin();}
		const_iterator end() const{return _data.end();}
};

#endif
