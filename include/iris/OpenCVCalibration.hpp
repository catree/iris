////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is part of iris, a lightweight C++ camera calibration library    //
//                                                                            //
// Copyright (C) 2012 Alexandru Duliu                                         //
//                                                                            //
// iris is free software; you can redistribute it and/or                      //
// modify it under the terms of the GNU Lesser General Public                 //
// License as published by the Free Software Foundation; either               //
// version 3 of the License, or (at your option) any later version.           //
//                                                                            //
// iris is distributed in the hope that it will be useful, but WITHOUT ANY    //
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS  //
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the //
// GNU General Public License for more details.                               //
//                                                                            //
// You should have received a copy of the GNU Lesser General Public           //
// License along with iris. If not, see <http://www.gnu.org/licenses/>.       //
//                                                                            //
///////////////////////////////////////////////////////////////////////////////

#pragma once

/*
 * OpenCVCalibration.hpp
 *
 *  Created on: Mar 14, 2011
 *      Author: duliu
 */

#include <iris/Calibration.hpp>

namespace iris {

class OpenCVCalibration : public Calibration
{
public:
    OpenCVCalibration();
    virtual ~OpenCVCalibration();

    void configure( bool fixPrincipalPoint,
                    bool fixAspectRatio,
                    bool tangentialDistortion );

    virtual void calibrate();

 protected:
    void calibrateCamera( Calibration::Camera& cam );

    int flags();

protected:
    bool m_fixPrincipalPoint;
    bool m_fixAspectRatio;
    bool m_tangentialDistortion;

};

} // end namespace iris