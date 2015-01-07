/*   This file is part of rl-lib
 *
 *   Copyright (C) 2010,  Supelec
 *
 *   Author : Herve Frezza-Buet and Matthieu Geist
 *
 *   Contributor :
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License (GPL) as published by the Free Software Foundation; either
 *   version 3 of the License, or any later version.
 *   
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *   Contact : Herve.Frezza-Buet@supelec.fr Matthieu.Geist@supelec.fr
 *
 */

#pragma once

#include <cstdlib>
#include <cmath>
#include <sstream>
#include <fstream>

#include <rlAlgo.hpp>
#include <rlException.hpp>

namespace rl {
  namespace problem {
    namespace inverted_pendulum {
      
      // The action space
      typedef enum Action {actionNone,
			   actionLeft,
			   actionRight} Action;
      
      // some exceptions for state and action consistancy
      class BadAction : public rl::exception::Any {
      public:
	BadAction(std::string comment) 
	  : Any(std::string("Bad action performed : ")+comment) {} 
      };
      
      class BadState : public rl::exception::Any {
      public:
	BadState(std::string comment) 
	  : Any(std::string("Bad state found : ")+comment) {} 
      };


      // Inverted pendulum parameters
      class DefaultParam {
      public:
	// This is the amplitude of the noise (relative) applied to the action.
	inline static double actionNoise(void)        {return 0.20;}
	// This is the noise of angle perturbation from the equilibrium state at initialization.
	inline static double angleInitNoise(void)     {return 1e-3;}
	// This is the noise of speed perturbation from the equilibrium state at initialization.
	inline static double speedInitNoise(void)    {return 1e-3;}
	
      };

      // This is the phase space
      template<typename PARAM>
      class Phase {
      public:

	typedef PARAM param_type;

	double angle,speed;

	Phase(void) {random();}
	Phase(const Phase<param_type>& copy) : angle(copy.angle), speed(copy.speed) {}
	Phase(double p, double s) : angle(p), speed(s) {}
	~Phase(void) {}
	Phase<param_type>& operator=(const Phase<param_type>& copy) {
	  if(this != &copy) {
	    angle = copy.angle;
	    speed    = copy.speed;
	  }
	  return *this;
	}

	void check(std::string message) const {
	  if(fabs(angle) > M_PI_2) {
	    std::ostringstream ostr;
	    ostr << "inverted_pendulum::Phase::check : At angle = " << angle << " : " << message;
	    throw BadState(ostr.str());
	  }
	}

	// This returns a random phase around the equilibrium
	void random(void) {
	  angle = param_type::angleInitNoise()*rl::random::uniform(-1,1);
	  speed = param_type::speedInitNoise()*rl::random::uniform(-1,1);
	}

      };

      /**
       * Inverted pendulum simulator  
       * @author <a href="mailto:Herve.Frezza-Buet@supelec.fr">Herve.Frezza-Buet@supelec.fr</a>
       */
      template<typename INVERTED_PENDULUM_PARAM>
      class Simulator {
	
      public:

	typedef INVERTED_PENDULUM_PARAM param_type;

	typedef Phase<param_type>  phase_type;
	typedef phase_type         observation_type;
	typedef Action             action_type;
	typedef double             reward_type;

      private:

	phase_type current_state;
	double r;

	// Standard parameters
	class Param {
	public:
	  static double g(void)        {return 9.8;}
	  static double m(void)        {return 2.0;}
	  static double M(void)        {return 8.0;}
	  static double l(void)        {return 0.5;}
	  static double a(void)        {return 1.0/(m()+M());}
	  static double strength(void) {return 50.0;}
	  static double tau(void)      {return  0.1;}
	  static double aml(void)      {return a()*m()*l();}
	};

      public:

	void setPhase(const phase_type& s) {
	  current_state = s;
	  current_state.check("in setPhase");
	}

	const observation_type& sense(void) const {
	  current_state.check("in sense");
	  return current_state;
	}

	void timeStep(const action_type& a) {
	  double aa;
	  double acc,cphi;

	  switch(a) {
	  case actionRight:
	    aa = 1;
	    break;
	  case actionLeft: 
	    aa = -1;
	    break;
	  case actionNone:
	    aa = 0;
	    break;
	  default:
	    std::ostringstream ostr;
	    ostr << "inverted_pendulum::Simulator::timeStep(" << a << ")";
	    throw BadAction(ostr.str());
	  }
	  
	  aa += param_type::actionNoise()*rl::random::uniform(-1,1);
	  aa *= Param::strength();
	        
	  cphi = cos(current_state.angle);
	  acc = ( Param::g()*sin(current_state.angle) 
		  - .5*Param::aml()*sin(2*current_state.angle)*current_state.speed*current_state.speed 
		  - Param::a()*cphi*aa )
	    / ( 4*Param::l()/3.0 - Param::aml()*cphi*cphi );

	  current_state.angle += current_state.speed*Param::tau();
	  current_state.speed += acc*Param::tau();

	  if(fabs(current_state.angle)>M_PI_2) {
	    r = -1;
	    throw rl::exception::Terminal("Pendulum has fallen down");
	  }
	  r = 0;
	}

	reward_type reward(void) const {
	  return r;
	}

	Simulator(void) : current_state(), r(0) {}
	Simulator(const Simulator& copy) 
	  : current_state(copy.current_state),
	    r(copy.r) {}
	~Simulator(void) {}

	Simulator& operator=(const Simulator& copy) {
	  if(this != &copy) 
	    current_state = copy.current_state;
	  return *this;
	}
      };
    }
  }
}
