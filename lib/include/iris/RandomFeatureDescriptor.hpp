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
 * RandomFeatureDescriptor.hpp
 *
 *  Created on: Jul 26, 2012
 *      Author: duliu
 */

#include <algorithm>
#include <limits>

#include <iris/util.hpp>
#include <Eigen/Core>

namespace iris
{

template <size_t M, size_t N, size_t K>
class RandomFeatureDescriptor
{
public:
    typedef std::vector<double> FeatureVector;

    struct Point
    {
        Eigen::Vector2d pos;
        size_t index;
        std::vector<Eigen::Vector2d> neighbors;
    };


public:
    RandomFeatureDescriptor();
    virtual ~RandomFeatureDescriptor();

    void operator() ( const std::vector<Eigen::Vector2d>& points, bool generateShiftPermutations=false );

    Pose_d operator& ( const RandomFeatureDescriptor& rfd ) const;

protected:
    void describePoint( Point& point, bool generateShiftPermutations );

    double computeDescriptor( const Eigen::Vector2d& c, const std::vector<Eigen::Vector2d>& n ); // center & neighbors

protected:
    // input data
    std::vector<Eigen::Vector2d> m_points;

    // point descriptors
    std::vector<Point> m_pd;
    std::vector<FeatureVector> m_featureVectors;

    // prebacked combinations
    std::vector< std::vector<size_t> > m_mCn;
    std::vector< std::vector<size_t> > m_nCk;
    size_t m_featureVectorsPerPoint;
};


/////
// Implementation
///

template <size_t M, size_t N, size_t K>
inline RandomFeatureDescriptor<M,N,K>::RandomFeatureDescriptor()
{
    m_mCn = possible_combinations<M,N>();
    m_nCk = possible_combinations<N,K>();
    m_featureVectorsPerPoint = m_mCn.size();
}


template <size_t M, size_t N, size_t K>
inline RandomFeatureDescriptor<M,N,K>::~RandomFeatureDescriptor()
{
    // TODO Auto-generated destructor stub
}


template <size_t M, size_t N, size_t K>
inline void RandomFeatureDescriptor<M,N,K>::operator() ( const std::vector<Eigen::Vector2d>& points, bool generateShiftPermutations )
{
    // init stuff
    m_points = points;
    Eigen::Vector2d up(0,1);
    m_pd.clear();
    m_pd.reserve( m_points.size() );
    if( generateShiftPermutations )
        m_featureVectorsPerPoint *= K;
    m_featureVectors.clear();
    m_featureVectors.reserve( m_points.size() * m_featureVectorsPerPoint );

    // init flann
    cv::Mat_<double> pointsCV( static_cast<int>(points.size()), 2 );
    for( size_t i=0; i<points.size(); i++ )
    {
        pointsCV( i, 0 ) = points[i](0);
        pointsCV( i, 1 ) = points[i](1);
    }
    cv::flann::GenericIndex< cv::flann::L2_Simple<double> > pointsFlann( pointsCV, cvflann::KDTreeIndexParams(5) );

    // generate the descriptor tree for each point
    for( size_t p=0; p<m_points.size(); p++ )
    {
        // get the M nearest neighbors of point
        cv::Mat_<int> nearestM( 1, M+1 );
        cv::Mat_<double> distsM( 1, M+1 );
        pointsFlann.knnSearch( pointsCV.row(p).clone(), nearestM, distsM, M+1, cvflann::SearchParams(128) );

        // copy points
        std::vector<Eigen::Vector2d> nearestPoints(M);
        for( size_t m=0; m<M; m++ )
            nearestPoints[m] = m_points[ nearestM(m+1) ];

        // sort counter clockwise
        counter_clockwise_comparisson<double> ccc( m_points[p], up );
        std::sort( nearestPoints.begin(), nearestPoints.end(), ccc );

        // initialize the point
        Point point;
        point.pos = m_points[p];
        point.index = p;
        point.neighbors = nearestPoints;

        // compute the feature vectors for this point
        describePoint( point, generateShiftPermutations );

        // add the point
        m_pd.push_back( point );
    }
}


template <size_t M, size_t N, size_t K>
inline Pose_d RandomFeatureDescriptor<M,N,K>::operator& ( const RandomFeatureDescriptor<M,N,K>& rfd ) const
{
    // addemble the descriptors
    cv::Mat_<float> queryFVs = vector2cv<float>(m_featureVectors);
    cv::Mat_<float> trainFVs = vector2cv<float>(rfd.m_featureVectors);

//    cv::flann::GenericIndex< cv::flann::L2<double> > flann( trainFVs, cvflann::KDTreeIndexParams(5) );

//    // get the M nearest neighbors of point
//    cv::Mat_<int> nearestM( int(m_featureVectors.size()), 1 );
//    cv::Mat_<double> distsM( int(m_featureVectors.size()), 1 );
//    flann.knnSearch( queryFVs, nearestM, distsM, 1, cvflann::SearchParams(128) );

//    std::cout << std::setprecision(2) << queryFVs << std::endl << std::endl << std::endl;
//    std::cout << std::setprecision(2) << trainFVs << std::endl << std::endl << std::endl;

    // match the feature vectors
    cv::FlannBasedMatcher matcher;
    //cv::BruteForceMatcher<cv::L2<float> > matcher;
    std::vector< cv::DMatch > matches;
    matcher.match( queryFVs, trainFVs, matches );

    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < queryFVs.rows; i++ )
    { double dist = matches[i].distance;
      if( dist > 0 && dist < min_dist ) min_dist = dist;
      if( dist > max_dist ) max_dist = dist;
    }

    printf("-- Max dist : %f \n", max_dist );
    printf("-- Min dist : %f \n", min_dist );

    //-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist )
    //-- PS.- radiusMatch can also be used here.
    std::vector< cv::DMatch > good_matches;

    for( int i = 0; i < queryFVs.rows; i++ )
    { if( matches[i].distance > 0 && matches[i].distance < 2*min_dist )
      { good_matches.push_back( matches[i]); }
    }

    if( good_matches.size() == 0 )
        std::cout << "RandomFeatureDescriptor: NO good Matches." << std::endl;

    for( int i = 0; i < good_matches.size(); i++ )
    { printf( "-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx ); }

    // assemble the pose
    Pose_d pose;

    return pose;
}


template <size_t M, size_t N, size_t K>
inline void RandomFeatureDescriptor<M,N,K>::describePoint( Point& point, bool generateShiftPermutations )
{
    // run over all N combinations
    size_t shiftCount = generateShiftPermutations ? K : 1;
    for( size_t n=0; n<m_mCn.size(); n++ )
    {
        /////
        // WARNING: we are now leaving Kansas
        ///

        // reserve enough for all feature vectors
        std::vector<FeatureVector> fvs(shiftCount);
        for( size_t i=0; i<shiftCount; i++ )
            fvs[i].reserve( m_featureVectorsPerPoint / (m_mCn.size() * shiftCount) );

        // run over all combinations of K elements from mCn
        for( size_t k=0; k<m_nCk.size(); k++ )
        {
            // assemble the points
            std::vector<Eigen::Vector2d> points;
            points.reserve( K );
            for( size_t i=0; i<K; i++ )
                points.push_back( point.neighbors[ m_mCn[n][ m_nCk[k][i] ] ] );

            // check if shift permutations are required
            if( generateShiftPermutations )
            {
                // generate shift combinations
                std::vector< std::vector<Eigen::Vector2d> > shift_points = shift_combinations( points );

                // run over all shift combinations and generate descriptors
                for( size_t i=0; i<K; i++ )
                    fvs[i].push_back( computeDescriptor( point.pos, shift_points[i] ) );
            }
            else
                fvs[0].push_back( computeDescriptor( point.pos, points ) );
        }

        // add the feature vector(s)
        for( size_t i=0; i<fvs.size(); i++ )
            m_featureVectors.push_back(fvs[i]);
    }
}


template <size_t M, size_t N, size_t K>
inline double RandomFeatureDescriptor<M,N,K>::computeDescriptor( const Eigen::Vector2d& c, const std::vector<Eigen::Vector2d>& n )
{
    // compute the cross ratio of five coplanar points
    // area(A,B,C)*area(A,D,E) / area(A,B,D)*area(A,C,E)

    double result = area( c, n[0], n[2] ) * area( c, n[1], n[3] );

    if( fabs( result ) < std::numeric_limits<double>::epsilon() )
        return 0;
    else
        return ( area( c, n[0], n[1] ) * area( c, n[2], n[3] ) ) / result;
}


} // end namespace iris
