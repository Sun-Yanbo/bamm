#include "EventRateProposal.h"
#include "Random.h"
#include "Settings.h"
#include "Model.h"
#include "Prior.h"

#include <algorithm>


EventRateProposal::EventRateProposal
    (Random& random, Settings& settings, Model& model, Prior& prior) :
        _random(random), _settings(settings), _model(model), _prior(prior)
{
    _weight = _settings.get<double>("updateRateEventRate");
    _updateEventRateScale = _settings.get<double>("updateEventRateScale");
}


void EventRateProposal::propose()
{
    _currentEventRate = _model.getEventRate();

    _cterm = std::exp(_updateEventRateScale * (_random.uniform() - 0.5));
    _proposedEventRate = _cterm * _currentEventRate;

    _model.setEventRate(_proposedEventRate);
    
    
    
}


void EventRateProposal::accept()
{
}


void EventRateProposal::reject()
{
    _model.setEventRate(_currentEventRate);
}


double EventRateProposal::acceptanceRatio()
{
    double logPriorRatio = computeLogPriorRatio();
    double logQRatio = computeLogQRatio();

    double t = _model.getTemperatureMH();
    double logRatio = t * logPriorRatio + logQRatio;

    if (std::isfinite(logRatio)) {
        return std::min(1.0, std::exp(logRatio));
    } else {
        return 0.0;
    }
}


double EventRateProposal::computeLogPriorRatio()
{
    return _prior.poissonRatePrior(_proposedEventRate) -
           _prior.poissonRatePrior(_currentEventRate);
}


double EventRateProposal::computeLogQRatio()
{
    return std::log(_cterm);
}
