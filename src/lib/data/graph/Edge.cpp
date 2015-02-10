#include "data/graph/Edge.h"

#include <sstream>

#include "data/graph/Node.h"
#include "data/graph/token_component/TokenComponentAggregation.h"
#include "data/graph/token_component/TokenComponentAccess.h"
#include "utility/logging/logging.h"

Edge::Edge(EdgeType type, Node* from, Node* to)
	: m_type(type)
	, m_from(from)
	, m_to(to)
{
	m_from->addEdge(this);
	m_to->addEdge(this);

	checkType();
}

Edge::Edge(const Edge& other, Node* from, Node* to)
	: Token(other)
	, m_type(other.m_type)
	, m_from(from)
	, m_to(to)
{
	m_from->addEdge(this);
	m_to->addEdge(this);

	if (m_from == other.m_from || m_to == other.m_to ||
		m_from->getId() != other.m_from->getId() || m_to->getId() != other.m_to->getId())
	{
		LOG_ERROR("Nodes are not plain copies.");
	}

	checkType();
}

Edge::~Edge()
{
	m_from->removeEdge(this);
	m_to->removeEdge(this);
}

Edge::EdgeType Edge::getType() const
{
	return m_type;
}

bool Edge::isType(EdgeTypeMask mask) const
{
	return (m_type & mask) > 0;
}

Node* Edge::getFrom() const
{
	return m_from;
}

Node* Edge::getTo() const
{
	return m_to;
}

std::string Edge::getName() const
{
	return getTypeString() + ":" + getFrom()->getFullName() + "->" + getTo()->getFullName();
}

bool Edge::isNode() const
{
	return false;
}

bool Edge::isEdge() const
{
	return true;
}

void Edge::addComponentAggregation(std::shared_ptr<TokenComponentAggregation> component)
{
	if (getComponent<TokenComponentAggregation>())
	{
		LOG_ERROR("TokenComponentAggregation has been set before!");
	}
	else if (m_type != EDGE_AGGREGATION)
	{
		LOG_ERROR("TokenComponentAggregation can't be set on edge of type: " + getTypeString());
	}
	else
	{
		addComponent(component);
	}
}

void Edge::addComponentAccess(std::shared_ptr<TokenComponentAccess> component)
{
	if (getComponent<TokenComponentAccess>())
	{
		LOG_ERROR("TokenComponentAccess has been set before!");
	}
	else if (m_type != EDGE_MEMBER && m_type != EDGE_INHERITANCE)
	{
		LOG_ERROR("TokenComponentAccess can't be set on edge of type: " + getTypeString());
	}
	else
	{
		addComponent(component);
	}
}

std::string Edge::getTypeString(EdgeType type) const
{
	switch (type)
	{
	case EDGE_MEMBER:
		return "child";
	case EDGE_TYPE_OF:
		return "type_use";
	case EDGE_RETURN_TYPE_OF:
		return "return_type";
	case EDGE_PARAMETER_TYPE_OF:
		return "parameter_type";
	case EDGE_TYPE_USAGE:
		return "type_usage";
	case EDGE_INHERITANCE:
		return "inheritance";
	case EDGE_OVERRIDE:
		return "override";
	case EDGE_CALL:
		return "call";
	case EDGE_USAGE:
		return "usage";
	case EDGE_TYPEDEF_OF:
		return "typedef";
	case EDGE_TEMPLATE_PARAMETER_OF:
		return "template parameter";
	case EDGE_TEMPLATE_ARGUMENT_OF:
		return "template argument";
	case EDGE_TEMPLATE_DEFAULT_ARGUMENT_OF:
		return "template default argument";
	case EDGE_TEMPLATE_SPECIALIZATION_OF:
		return "template specialization";
	case EDGE_AGGREGATION:
		return "aggregation";
	}
	return "";
}

std::string Edge::getTypeString() const
{
	return getTypeString(m_type);
}

std::string Edge::getAsString() const
{
	std::stringstream str;
	str << "[" << getId() << "] " << getTypeString() << ": \"" << m_from->getName() << "\" -> \"" + m_to->getName() << "\"";

	TokenComponentAccess* access = getComponent<TokenComponentAccess>();
	if (access)
	{
		str << " " << access->getAccessString();
	}

	TokenComponentAggregation* aggregation = getComponent<TokenComponentAggregation>();
	if (aggregation)
	{
		str << " " << aggregation->getAggregationCount();
	}

	return str.str();
}

std::ostream& operator<<(std::ostream& ostream, const Edge& edge)
{
	ostream << edge.getAsString();
	return ostream;
}

bool Edge::checkType() const
{
	Node::NodeTypeMask complexTypeMask = Node::NODE_UNDEFINED_TYPE | Node::NODE_CLASS | Node::NODE_STRUCT;
	Node::NodeTypeMask typeMask = Node::NODE_UNDEFINED | Node::NODE_ENUM | Node::NODE_TYPEDEF | complexTypeMask;
	Node::NodeTypeMask variableMask = Node::NODE_UNDEFINED | Node::NODE_UNDEFINED_VARIABLE | Node::NODE_GLOBAL_VARIABLE | Node::NODE_FIELD;
	Node::NodeTypeMask functionMask = Node::NODE_UNDEFINED_FUNCTION | Node::NODE_FUNCTION | Node::NODE_METHOD;

	switch (m_type)
	{
	case EDGE_MEMBER:
		if (!m_from->isType(typeMask | Node::NODE_NAMESPACE) ||
			(!m_from->isType(Node::NODE_UNDEFINED | Node::NODE_NAMESPACE) && m_to->isType(Node::NODE_NAMESPACE)) ||
			(m_from->isType(Node::NODE_ENUM) && !m_to->isType(Node::NODE_FIELD)))
		{
			break;
		}
		return true;

	case EDGE_TYPE_OF:
		if (!m_from->isType(variableMask) || !m_to->isType(typeMask))
		{
			break;
		}
		return true;

	case EDGE_RETURN_TYPE_OF:
	case EDGE_PARAMETER_TYPE_OF:
	case EDGE_TYPE_USAGE:
		if (!m_from->isType(functionMask) || !m_to->isType(typeMask))
		{
			break;
		}
		return true;

	case EDGE_INHERITANCE:
		if (!m_from->isType(complexTypeMask) || !m_to->isType(complexTypeMask))
		{
			break;
		}
		return true;

	case EDGE_OVERRIDE:
		if (!m_from->isType(Node::NODE_UNDEFINED_FUNCTION | Node::NODE_METHOD) ||
			!m_to->isType(Node::NODE_UNDEFINED_FUNCTION | Node::NODE_METHOD))
		{
			break;
		}
		return true;

	case EDGE_CALL:
		if (!m_from->isType(variableMask | functionMask) || !m_to->isType(functionMask))
		{
			break;
		}
		return true;

	case EDGE_USAGE:
		if (!m_from->isType(functionMask) || !m_to->isType(variableMask))
		{
			break;
		}
		return true;

	case EDGE_TYPEDEF_OF:
		if (!m_from->isType(Node::NODE_TYPEDEF) || !m_to->isType(typeMask))
		{
			break;
		}
		return true;

	case EDGE_TEMPLATE_PARAMETER_OF:
		if (!m_from->isType(Node::NODE_TEMPLATE_PARAMETER_TYPE) || !m_to->isType(typeMask | functionMask))
		{
			break;
		}
		return true;

	case EDGE_TEMPLATE_ARGUMENT_OF:
	case EDGE_TEMPLATE_DEFAULT_ARGUMENT_OF:
		if (!m_from->isType(typeMask) || !m_to->isType(typeMask))
		{
			break;
		}
		return true;

	case EDGE_TEMPLATE_SPECIALIZATION_OF:
		if (!m_from->isType(typeMask | functionMask) || !m_to->isType(typeMask | functionMask))
		{
			break;
		}
		return true;

	case EDGE_AGGREGATION:
		if (!m_from->isType(typeMask | variableMask | functionMask) || !m_to->isType(typeMask | variableMask | functionMask))
		{
			break;
		}
		return true;
	}

	LOG_ERROR_STREAM(
		<< "Edge " << getTypeString()
		<< " can't go from Node " << m_from->getTypeString()
		<< " to Node " << m_to->getTypeString()
	);
	return false;
}
