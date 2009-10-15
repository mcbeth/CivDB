#ifndef FACTORY_H__
#define FACTORY_H__

#include <map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>

template<typename Func> class FactoryOwner
{
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
		private:
				struct less
				{
					bool operator()(const std::string &l, const std::string &r) const
					{
						return boost::algorithm::ilexicographical_compare(l,r);
					}
				};
				std::map<std::string, Func, less> _data;
};

#endif
