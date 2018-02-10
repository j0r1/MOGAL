#include "distparams.h"

DistGAFactoryParams::DistGAFactoryParams(double x, double y, double width)
{
	m_x = x;
	m_y = y;
	m_width = width;
}

DistGAFactoryParams::~DistGAFactoryParams()
{
}

bool DistGAFactoryParams::write(serut::SerializationInterface &si) const
{
	if (!si.writeDouble(m_x))
	{
		setErrorString("Couldn't write X coordinate for test function");
		return false;
	}
	if (!si.writeDouble(m_y))
	{
		setErrorString("Couldn't write Y coordinate for test function");
		return false;
	}
	if (!si.writeDouble(m_width))
	{
		setErrorString("Couldn't write initial search width");
		return false;
	}
	return true;
}

bool DistGAFactoryParams::read(serut::SerializationInterface &si)
{
	if (!si.readDouble(&m_x))
	{
		setErrorString("Couldn't read X coordinate for test function");
		return false;
	}
	if (!si.readDouble(&m_y))
	{
		setErrorString("Couldn't read Y coordinate for test function");
		return false;
	}
	if (!si.readDouble(&m_width))
	{
		setErrorString("Couldn't read initial search width");
		return false;
	}

	return true;
}


