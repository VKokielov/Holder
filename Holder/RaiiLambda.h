#pragma once

namespace holder::lib
{

	template<typename Enter, typename Exit>
	class RaiiLambda
	{
	public:
		RaiiLambda(Enter&& enter, Exit&& exit)
			:m_exit(std::move(exit))
		{
			enter();
			m_inside = true;
		}

		void Bail()
		{
			if (m_inside)
			{
				m_exit();
				m_inside = false;
			}
		}

		~RaiiLambda()
		{
			Bail();
		}

	private:
		bool m_inside{ true };
		Exit m_exit;
	};

}