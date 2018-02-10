#ifndef DISTPARAMS_H

#define DISTPARAMS_H

#include <mogal/gafactory.h>

class DistGAFactoryParams : public mogal::GAFactoryParams
{
public:
	DistGAFactoryParams(double x, double y, double width);
	~DistGAFactoryParams();

	double getX() const								{ return m_x; }
	double getY() const								{ return m_y; }
	double getWidth() const								{ return m_width; }

	bool write(serut::SerializationInterface &si) const;
	bool read(serut::SerializationInterface &si);
private:
	double m_x, m_y, m_width;
};

#endif // DISTPARAMS_H

